// Fill out your copyright notice in the Description page of Project Settings.

#include "SFDragonCombatComponent.h"
#include "AbilitySystemGlobals.h"
#include "AIController.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Enemy/SFPrimarySet_Enemy.h"
#include "AbilitySystem/Attributes/SFPrimarySet.h"
#include "AI/SFAIGameplayTags.h"
#include "AI/Controller/SFBaseAIController.h"
#include "AI/StateMachine/SFStateMachine.h"
#include "AI/StateMachine/State/Boss_Dragon/SFPhaseCondition.h"
#include "Character/SFCharacterBase.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Interface/SFEnemyAbilityInterface.h"

USFDragonCombatComponent::USFDragonCombatComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USFDragonCombatComponent::InitializeCombatComponent()
{
	Super::InitializeCombatComponent();
	
	AAIController* AIController = Cast<AAIController>(GetOwner());
	if (!AIController) return;

	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn) return; 
	
	if (!CachedASC)
	{
		CachedASC = Cast<USFAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ControlledPawn));
	}

	if (CachedASC)
	{
		const USFPrimarySet_Enemy* PrimarySet = CachedASC->GetSet<USFPrimarySet_Enemy>();
		if (PrimarySet)
		{
			USFPrimarySet_Enemy* Set = const_cast<USFPrimarySet_Enemy*>(PrimarySet);
			Set->OnTakeDamageDelegate.RemoveDynamic(this, &ThisClass::AddThreat);
			Set->OnTakeDamageDelegate.AddDynamic(this, &ThisClass::AddThreat);
		}
		CachedASC->GetGameplayAttributeValueChangeDelegate(USFPrimarySet::GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
	}
	USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(ControlledPawn);
	if (PawnExtComp)
	{
		const USFPawnData* PawnData = PawnExtComp->GetPawnData<USFPawnData>();
		if (const USFEnemyData* EnemyData = Cast<USFEnemyData>(PawnData))
		{
			if (EnemyData->PhaseData.Num() > 0)
			{
				PhaseData = EnemyData->PhaseData;  
			}
			TriggerPhase.Reset();
		}
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

void USFDragonCombatComponent::CleanupThreatMap()
{
	
	for (auto It = ThreatMap.CreateIterator(); It; ++It)
	{
		AActor* ActorKey = It.Key();
		
		if (!IsValid(ActorKey)) 
		{
			It.RemoveCurrent();
			continue;
		}
		if (!IsValidTarget(ActorKey))
		{
			It.RemoveCurrent();
		}
	}
}

AActor* USFDragonCombatComponent::GetHighestThreatActor()
{
    if (ThreatMap.Num() == 0) return nullptr;

    // 1. 반드시 nullptr와 0.f로 시작하여 새로 선출합니다.
    AActor* HighestThreatActor = nullptr;
    float HighestValue = -1.f; 

    for (auto& ThreatPair : ThreatMap)
    {
        if (!IsValidTarget(ThreatPair.Key)) continue;

        if (ThreatPair.Value > HighestValue)
        {
            HighestValue = ThreatPair.Value;
            HighestThreatActor = ThreatPair.Key;
        }
    }

    return HighestThreatActor;
}

void USFDragonCombatComponent::EvaluateTarget()
{
    CleanupThreatMap();
    AActor* NewTarget = GetHighestThreatActor();
	
    if (NewTarget)
    {
        // 타겟을 찾은 경우
        if (GetCurrentTarget() != NewTarget)
        {
            UpdateTargetActor(NewTarget);
        }

        CurrentTargetState = EBossTargetState::Locked;
        LastValidTargetTime = GetWorld()->GetTimeSeconds();
        UpdateSpatialData();
        return; 
    }
	
	
    if (GetCurrentTarget() && CurrentTargetState == EBossTargetState::Locked)
    {
        if (ShouldForceReleaseTarget(GetCurrentTarget()))
        {
            UpdateTargetActor(nullptr);
            CurrentTargetState = EBossTargetState::None;
            return;
        }

        CurrentTargetState = EBossTargetState::Grace;
        UE_LOG(LogTemp, Log, TEXT("[Dragon] Target lost, entering Grace period."));
        return;
    }

    //  Grace 기간 만료 체크
	if (CurrentTargetState == EBossTargetState::Grace)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastValidTargetTime >= TargetGraceDuration)
		{
			UpdateTargetActor(nullptr);
			CurrentTargetState = EBossTargetState::None;

			CurrentZone = EBossAttackZone::None;
			CachedDistance = 0.f;
			CachedAngle = 0.f;

			UE_LOG(LogTemp, Log, TEXT("[Dragon] Grace expired → Target cleared"));
		}
	}

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
	
	TArray<FGameplayTag> Candidates;
	TArray<float> Weights;
	float TotalWeight = 0.f;

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		UGameplayAbility* Ability = Spec.Ability;
		if (!Ability)
		{
			continue;
		}

		FGameplayTagContainer AllTags;
		AllTags.AppendTags(Ability->AbilityTags);
		AllTags.AppendTags(Ability->GetAssetTags());
		
		if (!AllTags.HasAny(SearchTags))
		{
			continue;
		}

		const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();
		
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

		
		if (Score > 0.f)
		{
			FGameplayTag UniqueTag;
			for (const FGameplayTag& Tag : AllTags)
			{
				if (SearchTags.HasTagExact(Tag))
				{
					UniqueTag = Tag;
					break;
				}
			}
			
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
	
	// 1. 현재 후보 중 최고 점수
	float MaxScore = 0.f;
	for (float W : Weights)
	{
		if (W > MaxScore)
		{
			MaxScore = W;
		}
	}
	
	float ScoreThreshold = MaxScore * 0.85f; 
    
	TArray<int32> EliteIndices; // 살아남은 후보들의 인덱스 저장
	float EliteTotalWeight = 0.f;

	for (int32 i = 0; i < Weights.Num(); ++i)
	{
		if (Weights[i] >= ScoreThreshold)
		{
			EliteIndices.Add(i);
			EliteTotalWeight += Weights[i];
		}
	}
	
	float RandomValue = FMath::FRandRange(0.f, EliteTotalWeight);
	bool bFound = false;

	for (int32 Index : EliteIndices)
	{
		float Weight = Weights[Index];
		if (RandomValue <= Weight)
		{
			OutSelectedTag = Candidates[Index];
			bFound = true;
			break;
		}
		RandomValue -= Weight;
	}
	
	if (!bFound && EliteIndices.Num() > 0)
	{
		OutSelectedTag = Candidates[EliteIndices.Last()];
	}
	
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
            &USFDragonCombatComponent::EvaluateTarget,
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

bool USFDragonCombatComponent::IsValidTarget(AActor* Target) const
{
	
	if (!IsValid(Target)) return false;
	
	ASFCharacterBase* SFCharacter = Cast<ASFCharacterBase>(Target);
	if (!SFCharacter) return false;

	USFAbilitySystemComponent* ASC = SFCharacter->GetSFAbilitySystemComponent();
	if (ASC && ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
	{
		return false;
	}

	return true;
}

bool USFDragonCombatComponent::ShouldForceReleaseTarget(AActor* Target) const
{
	if (!Target)
		return true;

	if (!Target->HasActorBegunPlay())
		return true;

	if (Target->IsPendingKillPending())
		return true;
	
	if (AActor* Owner = GetOwner())
	{
		float Distance = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation());
		if (Distance > MaxCombatRange)
			return true;
	}

	// Dead 체크
	ASFCharacterBase* SFCharacter = Cast<ASFCharacterBase>(Target);
	if (SFCharacter)
	{
		USFAbilitySystemComponent* ASC = SFCharacter->GetSFAbilitySystemComponent();
		if (ASC && ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
			return true;
	}

	return false;
}

void USFDragonCombatComponent::CheckPhaseTransitions()
{
	if (!HasAuthority() || PhaseData.IsEmpty()) return;

	AAIController* AIC = GetController<AAIController>();
	if (!AIC) return;
	AActor* Owner = AIC->GetPawn();
	if (!Owner) return;
	USFStateMachine* StateMachine = USFStateMachine::FindStateMachineComponent(Owner);
	if (!StateMachine) return;

	
	for (int32 i = PhaseData.Num() - 1; i >= 0; --i)
	{
		const FSFPhaseData& Step = PhaseData[i];
		
		if (TriggerPhase.Contains(Step)) continue;
		
		bool bAllConditionsMet = true;
		for (auto Condition : Step.Conditions)
		{
			if (Condition && !Condition->IsMet(CachedASC, GetOwner()))
			{
				bAllConditionsMet = false;
				break;
			}
		}
		
		if (bAllConditionsMet)
		{

			if (StateMachine->ActivateStateByTag(Step.TargetPhaseTag))
			{
				for (int32 j = 0; j <= i; ++j)
				{
					TriggerPhase.AddUnique(PhaseData[j]);
				}

				break;
			}
		}
	}
}

void USFDragonCombatComponent::OnHealthChanged(const FOnAttributeChangeData& OnAttributeChangeData)
{
	CheckPhaseTransitions();
}

