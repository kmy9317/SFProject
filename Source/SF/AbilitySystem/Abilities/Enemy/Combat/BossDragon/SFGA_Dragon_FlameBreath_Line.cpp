// Fill out your copyright notice in the Description page of Project Settings.

#include "SFGA_Dragon_FlameBreath_Line.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/SFCharacterBase.h"
#include "DrawDebugHelpers.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "Interface/SFAIControllerInterface.h"
#include "Animation/AnimInstance.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"

USFGA_Dragon_FlameBreath_Line::USFGA_Dragon_FlameBreath_Line()
{
	AbilityID = FName("Dragon_FlameBreath_Line");
	AttackType = EAttackType::Range;

	// Ability Tags
	AbilityTags.AddTag(SFGameplayTags::Ability_Dragon_FlameBreath_Line);

	// Cooldown Tag
	CoolDownTag = SFGameplayTags::Ability_Cooldown_Dragon_FlameBreath_Line;
}

void USFGA_Dragon_FlameBreath_Line::ActivateAbility( const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartCharging();
}


void USFGA_Dragon_FlameBreath_Line::StartCharging()
{
	AccumulatedInterruptDamage = 0.f;
	HitActors.Empty();

	PrimaryTarget = FindPrimaryTarget();
	if (!PrimaryTarget.IsValid())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (BreathMontage)
	{
		ChargeStartMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			BreathMontage,
			1.f,
			FName("ChargeStart"),
			true
		);

		if (ChargeStartMontageTask)
		{
			ChargeStartMontageTask->OnCompleted.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnChargeStartCompleted);
			ChargeStartMontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnMontageInterrupted);
			ChargeStartMontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnMontageCancelled);
			ChargeStartMontageTask->ReadyForActivation();
		}
	}

	if (CurrentActorInfo->IsNetAuthority())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			OnDamageReceivedHandle = ASC->OnGameplayEffectAppliedDelegateToSelf.AddUObject(
				this,
				&USFGA_Dragon_FlameBreath_Line::OnDamageReceivedDuringCharge
			);
		}
	}

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(
			ChargeTimerHandle,
			this,
			&USFGA_Dragon_FlameBreath_Line::TransitionToBreath,
			ChargeDuration,
			false
		);
	}
}

void USFGA_Dragon_FlameBreath_Line::OnChargeStartCompleted()
{
	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (Dragon && Dragon->GetMesh())
	{
		UAnimInstance* AnimInst = Dragon->GetMesh()->GetAnimInstance();
		if (AnimInst && BreathMontage)
		{
			AnimInst->Montage_JumpToSection(FName("ChargeLoop"), BreathMontage);
		}
	}
}

void USFGA_Dragon_FlameBreath_Line::TransitionToBreath()
{
	if (OnDamageReceivedHandle.IsValid())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			ASC->OnGameplayEffectAppliedDelegateToSelf.Remove(OnDamageReceivedHandle);
			OnDamageReceivedHandle.Reset();
		}
	}

	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (Dragon && Dragon->GetMesh())
	{
		UAnimInstance* AnimInst = Dragon->GetMesh()->GetAnimInstance();
		if (AnimInst && BreathMontage)
		{
			AnimInst->Montage_SetNextSection(
				FName("ChargeLoop"),
				FName("BreathStart"),
				BreathMontage
			);
			AnimInst->Montage_JumpToSection(FName("BreathStart"), BreathMontage);
		}
	}

	if (!CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(
			BreathTickTimer,
			this,
			&USFGA_Dragon_FlameBreath_Line::ApplyBreathDamage,
			BreathTickRate,
			true
		);

		World->GetTimerManager().SetTimer(
			BreathDurationTimer,
			this,
			&USFGA_Dragon_FlameBreath_Line::StopBreath,
			BreathDuration,
			false
		);
	}
}

void USFGA_Dragon_FlameBreath_Line::ApplyBreathDamage()
{
	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (!Dragon) return;

	FVector Start = Dragon->GetMesh()->GetSocketLocation(TEXT("JawSocket"));
	FVector Forward = Dragon->GetActorForwardVector();
	FVector End = Start + Forward * BreathRange;

	TArray<FHitResult> Hits;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Dragon);

	GetWorld()->LineTraceMultiByChannel(
		Hits,
		Start,
		End,
		ECollisionChannel::ECC_Pawn,
		QueryParams
	);

	if (bIsDebug)
	{
		DrawDebugLine(
			GetWorld(),
			Start,
			End,
			FColor::Red,
			false,
			BreathTickRate,
			0,
			5.f
		);
	}

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor) continue;

		if (GetAttitudeTowards(HitActor) != ETeamAttitude::Hostile)
			continue;

		if (HitActors.Contains(HitActor))
			continue;

		HitActors.Add(HitActor);

		FGameplayEffectContextHandle EffectContext =
			MakeEffectContext(CurrentSpecHandle, CurrentActorInfo);
		EffectContext.AddHitResult(Hit);

		ApplyRawDamageToTarget(HitActor, BreathDamagePerTick,EffectContext);

		// Pressure 적용 (ISFDragonPressureInterface) - 첫 히트 시에만
		ApplyPressureToTarget(HitActor);

		if (bIsDebug)
		{
			DrawDebugSphere(
				GetWorld(),
				Hit.ImpactPoint,
				50.f,
				12,
				FColor::Orange,
				false,
				BreathTickRate
			);
		}
	}

	HitActors.Empty();
}


void USFGA_Dragon_FlameBreath_Line::StopBreath()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(BreathTickTimer);
	}

	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (Dragon && Dragon->GetMesh())
	{
		UAnimInstance* AnimInst = Dragon->GetMesh()->GetAnimInstance();
		if (AnimInst && BreathMontage)
		{
			AnimInst->Montage_SetNextSection(
				FName("BreathLoop"),
				FName("BreathEnd"),
				BreathMontage
			);

			AnimInst->Montage_JumpToSection(FName("BreathEnd"), BreathMontage);
		}
	}

	if (BreathMontage)
	{
		BreathEndMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			BreathMontage,
			1.f,
			FName("BreathEnd"),
			false
		);

		if (BreathEndMontageTask)
		{
			BreathEndMontageTask->OnCompleted.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnBreathEndCompleted);
			BreathEndMontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnMontageInterrupted);
			BreathEndMontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnMontageCancelled);
			BreathEndMontageTask->ReadyForActivation();
		}
	}
}


AActor* USFGA_Dragon_FlameBreath_Line::FindPrimaryTarget()
{
	AController* Controller = GetControllerFromActorInfo();
	if (!Controller)
	{
		return nullptr;
	}

	APawn* Pawn = Controller->GetPawn();
	if (!Pawn)
	{
		return nullptr;
	}

	AController* PawnController = Pawn->GetController();
	if (!PawnController)
	{
		return nullptr;
	}

	ISFAIControllerInterface* AIC = Cast<ISFAIControllerInterface>(PawnController);
	if (!AIC)
	{
		return nullptr;
	}

	USFEnemyCombatComponent* CombatComp = AIC->GetCombatComponent();
	if (!CombatComp)
	{
		return nullptr;
	}

	return CombatComp->GetCurrentTarget();
}


void USFGA_Dragon_FlameBreath_Line::OnDamageReceivedDuringCharge(UAbilitySystemComponent* Source, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle)
{
	float Damage = SpecApplied.GetSetByCallerMagnitude(SFGameplayTags::Data_Damage_BaseDamage, false, 0.f);

	if (Damage > 0.f)
	{
		AccumulatedInterruptDamage += Damage;

		if (AccumulatedInterruptDamage >= InterruptThreshold)
		{
			InterruptBreath();
		}
	}
}

void USFGA_Dragon_FlameBreath_Line::InterruptBreath()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(ChargeTimerHandle);
	}

	if (OnDamageReceivedHandle.IsValid())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			ASC->OnGameplayEffectAppliedDelegateToSelf.Remove(OnDamageReceivedHandle);
			OnDamageReceivedHandle.Reset();
		}
	}

	if (InterruptStaggerEffect)
	{
		UAbilitySystemComponent* DragonASC = GetAbilitySystemComponentFromActorInfo();
		if (DragonASC)
		{
			FGameplayEffectSpecHandle StaggerSpec = MakeOutgoingGameplayEffectSpec(InterruptStaggerEffect, 1);

			if (StaggerSpec.IsValid())
			{
				StaggerSpec.Data->SetSetByCallerMagnitude(
					SFGameplayTags::Data_Stagger_BaseStagger,
					InterruptStaggerDamage
				);

				DragonASC->ApplyGameplayEffectSpecToSelf(*StaggerSpec.Data.Get());
			}
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Dragon_FlameBreath_Line::OnBreathEndCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Dragon_FlameBreath_Line::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Dragon_FlameBreath_Line::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}


void USFGA_Dragon_FlameBreath_Line::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(ChargeTimerHandle);
		World->GetTimerManager().ClearTimer(BreathTickTimer);
		World->GetTimerManager().ClearTimer(BreathDurationTimer);
	}

	if (ChargeStartMontageTask)
	{
		ChargeStartMontageTask->EndTask();
		ChargeStartMontageTask = nullptr;
	}

	if (BreathEndMontageTask)
	{
		BreathEndMontageTask->EndTask();
		BreathEndMontageTask = nullptr;
	}

	if (OnDamageReceivedHandle.IsValid())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			ASC->OnGameplayEffectAppliedDelegateToSelf.Remove(OnDamageReceivedHandle);
			OnDamageReceivedHandle.Reset();
		}
	}

	PrimaryTarget.Reset();
	HitActors.Empty();
	AccumulatedInterruptDamage = 0.f;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

float USFGA_Dragon_FlameBreath_Line::CalcScoreModifier(const FEnemyAbilitySelectContext& Context) const
{
	// Boss Context로 캐스팅하여 Zone 정보 확인
	const FBossEnemyAbilitySelectContext* BossContext =
		static_cast<const FBossEnemyAbilitySelectContext*>(&Context);

	float Modifier = 0.f;

	// Mid Range와 Long Range에서 높은 점수
	if (BossContext && BossContext->Zone == EBossAttackZone::Mid)
	{
		Modifier += 800.f;  // Mid Range에서 우선 사용
	}

	if (BossContext && BossContext->Zone == EBossAttackZone::Long)
	{
		Modifier += 600.f;  // Long Range에서도 사용 가능
	}

	// 플레이어가 멀리 있을 때 추가 점수
	if (Context.DistanceToTarget > 2000.f)
	{
		Modifier += 500.f;
	}

	if (!Context.Target)
		return Modifier;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Context.Target);

	if (!TargetASC)
		return Modifier;

	// Pressure 없으면 증가 (압박 시작)
	if (!TargetASC->HasMatchingGameplayTag(SFGameplayTags::Dragon_Pressure_Forward) &&
		!TargetASC->HasMatchingGameplayTag(SFGameplayTags::Dragon_Pressure_Back))
	{
		Modifier += 300.f;
	}

	// 이미 Back Pressure 중이면 우선순위 감소 (중복 압박 방지)
	if (TargetASC->HasMatchingGameplayTag(SFGameplayTags::Dragon_Pressure_Back))
	{
		Modifier -= 400.f;  // 최소 0 이상 유지 (800 - 400 = 400)
	}

	return FMath::Max(Modifier, 0.f);  // 항상 0 이상
}
