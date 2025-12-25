// Fill out your copyright notice in the Description page of Project Settings.
#include "SFDragonCombatComponent.h"
#include "AbilitySystemGlobals.h"
#include "AIController.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Enemy/SFCombatSet_Enemy.h"
#include "AbilitySystem/Attributes/Enemy/SFPrimarySet_Enemy.h"
#include "AbilitySystem/Attributes/SFPrimarySet.h"
#include "AbilitySystem/Attributes/Hero/SFPrimarySet_Hero.h"
#include "Character/SFCharacterBase.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Interface/SFEnemyAbilityInterface.h"


USFDragonCombatComponent::USFDragonCombatComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CurrentZone(EBossAttackZone::None)
	, CachedDistance(0.f)
	, CachedAngle(0.f)
	, PlayerHealthPercent(1.0f)

{
	PrimaryComponentTick.bCanEverTick = false;
}



void USFDragonCombatComponent::InitializeCombatComponent()
{
	Super::InitializeCombatComponent();

	AAIController* Controller = GetController<AAIController>();
	if (!Controller) return;

	CachedASC = Cast<USFAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Controller->GetPawn()));

	if (!CachedASC)
	{
		return;
	}

	const USFPrimarySet_Enemy* PrimarySet = CachedASC->GetSet<USFPrimarySet_Enemy>();
	if (PrimarySet)
	{
		USFPrimarySet_Enemy* Set = const_cast<USFPrimarySet_Enemy*>(PrimarySet);
		Set->OnTakeDamageDelegate.RemoveDynamic(this, &ThisClass::AddThreat);
		Set->OnTakeDamageDelegate.AddDynamic(this, &ThisClass::AddThreat);
	}

	StartSpatialUpdateTimer();
	StartStateMonitorTimer();
	StartThreatUpdateTimer();
}

void USFDragonCombatComponent::AddThreat(float ThreatValue, AActor* Actor)
{
	if (!Actor)
		return;

	if (ThreatMap.Contains(Actor))
	{
		ThreatMap[Actor] += ThreatValue;
	}
	else
	{
		ThreatMap.Add(Actor, ThreatValue);
	}
}

AActor* USFDragonCombatComponent::GetHighestThreatActor()
{
	if (ThreatMap.Num() == 0)
		return nullptr;

	AActor* HighestThreatActor = nullptr;
	float HighestThreatValue = 0.f;

	
	TArray<AActor*> InvalidTargets;

	for (auto& ThreatPair : ThreatMap)
	{
		
		if (!IsValidTarget(ThreatPair.Key))
		{
			InvalidTargets.Add(ThreatPair.Key);
			continue;
		}
		
		if (HighestThreatActor == nullptr || ThreatPair.Value > HighestThreatValue)
		{
			HighestThreatValue = ThreatPair.Value;
			HighestThreatActor = ThreatPair.Key;
		}
	}
	
	for (AActor* InvalidTarget : InvalidTargets)
	{
		ThreatMap.Remove(InvalidTarget);
	}

	return HighestThreatActor;
}

void USFDragonCombatComponent::UpdateTargetFromThreat()
{
	AActor* NewTarget = GetHighestThreatActor();
	
	if (CurrentTarget != NewTarget)
	{
		CurrentTarget = NewTarget;
		
		if (CurrentTarget)
		{
			UpdateSpatialData();
		}
		else
		{
			CurrentZone = EBossAttackZone::None;
			CachedDistance = 0.f;
			CachedAngle = 0.f;
		}
	}
}

EBossAttackZone USFDragonCombatComponent::GetTargetLocationZone() const
{
	return CurrentZone;
}

float USFDragonCombatComponent::GetDistanceToTarget() const
{
	return CachedDistance;
}

float USFDragonCombatComponent::GetAngleToTarget() const
{
	return CachedAngle;
}

bool USFDragonCombatComponent::SelectAbility(const FEnemyAbilitySelectContext& Context, const FGameplayTagContainer& SearchTags, FGameplayTag& OutSelectedTag)
{
	FBossEnemyAbilitySelectContext DragonContext;

	DragonContext.Self = Context.Self;
	DragonContext.Target = Context.Target;
	DragonContext.DistanceToTarget = CachedDistance;
	DragonContext.AngleToTarget = CachedAngle;

	DragonContext.PlayerHealthPercentage = GetPlayerHealthPercent();
	DragonContext.Zone = CurrentZone;
	
	OutSelectedTag = FGameplayTag();

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(DragonContext.Self);
	if (!ASC || SearchTags.IsEmpty())
	{
		return false;
	}

	FEnemyAbilitySelectContext ContextWithSpatialData = DragonContext;

	if (DragonContext.Self && DragonContext.Target)
	{
		if (DragonContext.DistanceToTarget == 0.f)
		{
			ContextWithSpatialData.DistanceToTarget = DragonContext.Self->GetDistanceTo(DragonContext.Target);
		}
		if (DragonContext.AngleToTarget == 0.f)
		{
			if (ASFCharacterBase* Owner = Cast<ASFCharacterBase>(DragonContext.Self))
			{
				const FVector ToTarget = (DragonContext.Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal();
				const float Dot = FVector::DotProduct(Owner->GetActorForwardVector(), ToTarget);
				ContextWithSpatialData.AngleToTarget = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));
			}
		}
	}

	// 후보군과 가중치를 저장할 배열 선언
	TArray<FGameplayTag> Candidates;
	TArray<float> Weights;
	float TotalWeight = 0.f;

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		UGameplayAbility* Ability = Spec.GetPrimaryInstance();
		if (!Ability)
		{
			continue;
		}

		FGameplayTagContainer AllTags;
		AllTags.AppendTags(Ability->AbilityTags);
		AllTags.AppendTags(Ability->GetAssetTags());


		// Ability가 SearchTags 중 어떤 태그라도 포함하는가?
		if (!AllTags.HasAny(SearchTags))
		{
			continue;
		}

		const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();

		// 활성화 가능 체크
		if (!Ability->CanActivateAbility(Spec.Handle, ActorInfo))
		{
			continue;
		}

		// 쿨타임 체크
		if (!Ability->CheckCooldown(Spec.Handle, ActorInfo))
		{
			continue;
		}

		ISFEnemyAbilityInterface* AIInterface = Cast<ISFEnemyAbilityInterface>(Ability);
		if (!AIInterface)
		{
			continue;
		}

		// Context에 Spec 전달
		FEnemyAbilitySelectContext ContextWithSpec = ContextWithSpatialData;
		ContextWithSpec.AbilitySpec = &Spec;

		float Score = AIInterface->CalcAIScore(ContextWithSpec);

		// 점수가 0보다 클 때만 후보에 등록
		if (Score > 0.f)
		{
			// SearchTags 와 정확히 매칭되는 태그만 추출
			FGameplayTag UniqueTag;
			for (const FGameplayTag& Tag : AllTags)
			{
				if (SearchTags.HasTagExact(Tag))
				{
					UniqueTag = Tag;
					break;
				}
			}

			//그래도 못찾으면 그냥 AbilityTags 내 첫 태그 사용
			if (!UniqueTag.IsValid())
			{
				if (Ability->AbilityTags.Num() > 0)
				{
					UniqueTag = Ability->AbilityTags.First();
				}
				else if (Ability->GetAssetTags().Num() > 0)
				{
					UniqueTag = Ability->GetAssetTags().First();
				}
			}

			// 후보 등록 - 이전 공격 제외 로직
			if (UniqueTag.IsValid())
			{
				// 이전에 사용한 공격이면 스킵 (후보가 하나도 없을 때는 허용)
				if (UniqueTag == LastSelectedAbilityTag && Candidates.Num() > 0)
				{
					continue;
				}

				Candidates.Add(UniqueTag);
				Weights.Add(Score);
				TotalWeight += Score;
			}
		}
	}

	if (Candidates.Num() == 0)
	{
		return false;
	}

	// 가중치 랜덤 선택 (Weighted Random)
	float RandomValue = FMath::FRandRange(0.f, TotalWeight);

	for (int32 i = 0; i < Candidates.Num(); ++i)
	{
		if (RandomValue <= Weights[i])
		{
			OutSelectedTag = Candidates[i];
			LastSelectedAbilityTag = OutSelectedTag; // 선택한 공격 기록
			return true;
		}
		RandomValue -= Weights[i];
	}

	// 혹시라도 루프를 빠져나오면 마지막 후보 선택
	OutSelectedTag = Candidates.Last();
	LastSelectedAbilityTag = OutSelectedTag; // 선택한 공격 기록
	return true;
}
	
// 공간 정보 갱신 
void USFDragonCombatComponent::UpdateSpatialData()
{
	if (!IsValid(CurrentTarget))
	{
		CurrentZone = EBossAttackZone::None;
		CachedDistance = 0.f;
		CachedAngle = 0.f;
		return;
	}

	AAIController* AIC = GetController<AAIController>();
	if (!AIC || !AIC->GetPawn())
		return;

	APawn* Dragon = AIC->GetPawn();
	
	CachedDistance = FVector::Dist(
		Dragon->GetActorLocation(),
		CurrentTarget->GetActorLocation()
	);
	
	FVector ToTarget = CurrentTarget->GetActorLocation() - Dragon->GetActorLocation();
	ToTarget.Z = 0.f;
	ToTarget.Normalize();

	FVector Forward = Dragon->GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();

	CachedAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Forward, ToTarget)));
	
	if (CachedDistance <= MeleeRange)
	{
		CurrentZone = EBossAttackZone::Melee;
	}
	else if (CachedDistance <= MidRange)
	{
		CurrentZone = EBossAttackZone::Mid;
	}
	else if (CachedDistance <= LongRange)
	{
		CurrentZone = EBossAttackZone::Long;
	}
	else
	{
		CurrentZone = EBossAttackZone::OutOfRange;
	}
}

void USFDragonCombatComponent::MonitorTargetState()
{
	if (!IsValid(CurrentTarget))
	{

		PlayerHealthPercent = 1.0f;
		return;
	}

	ASFCharacterBase* Player = Cast<ASFCharacterBase>(CurrentTarget);
	if (!Player)
		return;

	USFAbilitySystemComponent* PlayerASC = Player->GetSFAbilitySystemComponent();
	if (!PlayerASC)
		return;
	
	if (const USFPrimarySet* PrimarySet = PlayerASC->GetSet<USFPrimarySet>())
	{
		PlayerHealthPercent = PrimarySet->GetHealth() / FMath::Max(PrimarySet->GetMaxHealth(), 1.f);
	}
	

}

void USFDragonCombatComponent::StartSpatialUpdateTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			SpatialUpdateTimerHandle,
			this,
			&USFDragonCombatComponent::UpdateSpatialData,
			SpatialUpdateInterval,
			true  
		);
	}
}

void USFDragonCombatComponent::StopSpatialUpdateTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpatialUpdateTimerHandle);
	}
}

void USFDragonCombatComponent::StartStateMonitorTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			StateMonitorTimerHandle,
			this,
			&USFDragonCombatComponent::MonitorTargetState,
			StateMonitorInterval,
			true  
		);
	}
}

void USFDragonCombatComponent::StopStateMonitorTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StateMonitorTimerHandle);
	}
}

void USFDragonCombatComponent::StartThreatUpdateTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ThreatUpdateTimerHandle,
			this,
			&USFDragonCombatComponent::UpdateTargetFromThreat,
			ThreatUpdateInterval,
			true
		);
	}
}

void USFDragonCombatComponent::StopThreatUpdateTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ThreatUpdateTimerHandle);
	}
}

bool USFDragonCombatComponent::IsValidTarget(AActor* Target) const //현재 타깃이 존재하는지
{
	if (!IsValid(Target))
	{
		return false;
	}

	ASFCharacterBase* SFCharacter = Cast<ASFCharacterBase>(Target);
	if (!SFCharacter)
	{
		return false;
	}

	USFAbilitySystemComponent* ASC = SFCharacter->GetSFAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return false;
	}

	// 죽은 상태의 적은 타깃으로 선택되지 않음
	if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
	{
		return false;
	}
	return true;

}


