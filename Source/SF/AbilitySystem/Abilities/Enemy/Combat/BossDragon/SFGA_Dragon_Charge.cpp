// Fill out your copyright notice in the Description page of Project Settings.

#include "SFGA_Dragon_Charge.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_MoveToLocation.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Task/SFAbilityTask_MoveToTargetAndCheckDistance.h"
#include "Character/SFCharacterBase.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interface/SFEnemyAbilityInterface.h"
#include "AI/Controller/Dragon/SFDragonCombatComponent.h"

USFGA_Dragon_Charge::USFGA_Dragon_Charge()
{
	AbilityID= FName("Dragon_Charge");
	AttackType = EAttackType::Melee;

	AbilityTags.AddTag(SFGameplayTags::Ability_Dragon_Charge);
	CoolDownTag = SFGameplayTags::Ability_Cooldown_Dragon_Charge;
}

void USFGA_Dragon_Charge::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	AActor* Target = GetCurrentTarget();
	if (!Dragon || !Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	
	if (ChargeMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, ChargeMontage);

		if (MontageTask)
		{
			MontageTask->OnCompleted.AddDynamic(this, &USFGA_Dragon_Charge::OnMontageEnded);
			MontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_Charge::OnMontageEnded);
			MontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_Charge::OnMontageEnded);
			MontageTask->ReadyForActivation();
		}
	}

	if (!ActorInfo->IsNetAuthority())
	{
		return;
	}

	if (Dragon->GetMesh())
	{
		OriginalPawnResponse = Dragon->GetMesh()->GetCollisionResponseToChannel(ECC_Pawn);
		Dragon->GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Dragon->GetMesh()->OnComponentBeginOverlap.AddDynamic(
			this, &USFGA_Dragon_Charge::OnChargeOverlap);
	}

	const FVector Start = Dragon->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();
	const FVector Dir = (TargetLoc - Start).GetSafeNormal2D();
	FVector Loc = TargetLoc - (Dir * StopDistance);
	Loc.Z = Start.Z;

	USFAbilityTask_MoveToTargetAndCheckDistance* MoveTask =
		USFAbilityTask_MoveToTargetAndCheckDistance::MoveToTargetAndCheckDistance(
			this, Target,  StopDistance,ChargeDuration, ChargeSpeedCurve);

	if (MoveTask)
	{
		MoveTask->OnFinished.AddDynamic(this, &USFGA_Dragon_Charge::OnMoveFinished);
		MoveTask->ReadyForActivation();
	}
}


void USFGA_Dragon_Charge::FinishCharge(bool bCancelled)
{
	if (!IsActive())
		return;

	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (Dragon && Dragon->GetCharacterMovement())
	{
		Dragon->GetCharacterMovement()->StopMovementImmediately();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bCancelled);
}

void USFGA_Dragon_Charge::OnMoveFinished(bool bSuccess)
{
	FinishCharge(false);
}

void USFGA_Dragon_Charge::OnMoveCancelled()
{
	FinishCharge(true);
}

void USFGA_Dragon_Charge::OnMontageEnded()
{
	FinishCharge(false);
}


void USFGA_Dragon_Charge::EndAbility( const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,  bool bReplicateEndAbility,bool bWasCancelled)
{
	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (Dragon && Dragon->GetMesh())
	{
		Dragon->GetMesh()->OnComponentBeginOverlap.RemoveAll(this);
		Dragon->GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, OriginalPawnResponse);
	}

	HitActors.Empty();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


void USFGA_Dragon_Charge::OnChargeOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || HitActors.Contains(OtherActor))
		return;

	if (GetAttitudeTowards(OtherActor) != ETeamAttitude::Hostile)
		return;

	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (Dragon)
	{
		ApplyKnockBackToTarget(OtherActor, Dragon->GetActorLocation());
		HitActors.Add(OtherActor);
	}
}

float USFGA_Dragon_Charge::CalcScoreModifier(const FEnemyAbilitySelectContext& Context) const
{
	
	if (Context.DistanceToTarget < 1500.f)
	{
		return -1.f;  // 필터링 신호
	}

	
	const FBossEnemyAbilitySelectContext* BossContext =
		static_cast<const FBossEnemyAbilitySelectContext*>(&Context);

	float Modifier = 0.f;

	// OutOfRange일 때 최우선 순위 
	if (BossContext && BossContext->Zone == EBossAttackZone::OutOfRange)
	{
		Modifier += 2000.f;  // OutOfRange에서 Charge 최우선 선택
	}

	//  Long Range일 때 높은 점수 부여
	if (BossContext && BossContext->Zone == EBossAttackZone::Long)
	{
		Modifier += 1500.f;  // Long Range에서 Charge 우선 선택
	}

	// 조건 3: Mid Range에서도 사용 가능 
	if (BossContext && BossContext->Zone == EBossAttackZone::Mid)
	{
		Modifier += 500.f;  // Mid Range에서는 다른 스킬보다 낮은 우선순위
	}

	// 조건 4: 거리가 매우 멀 때 추가 점수
	if (Context.DistanceToTarget > 5000.f)
	{
		Modifier += 1000.f;  // 멀어졌을 때 높은 점수
	}

	if (!Context.Target)
		return Modifier;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Context.Target);

	if (!TargetASC)
		return Modifier;

	// Back Pressure 중이면 추가 점수 (도망가는 플레이어 추격)
	if (TargetASC->HasMatchingGameplayTag(SFGameplayTags::Dragon_Pressure_Back))
	{
		Modifier += 500.f;
	}

	return Modifier;
}
