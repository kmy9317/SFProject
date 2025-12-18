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

USFGA_Dragon_FlameBreath_Line::USFGA_Dragon_FlameBreath_Line()
{
	
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
	// TODO: Add GameplayTag to AvatarActor (e.g., State.Dragon.Charging)

	AccumulatedInterruptDamage = 0.f;
	HitActors.Empty();

	//타깃 설정 
	PrimaryTarget = FindPrimaryTarget();
	if (!PrimaryTarget.IsValid())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 몽타주 재생
	if (BreathMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this,
				NAME_None,
				BreathMontage,
				1.f,
				FName("ChargeStart"), 
				true
			);

		if (MontageTask)
		{
			MontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnMontageInterrupted);
			MontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnMontageCancelled);
			MontageTask->ReadyForActivation();
		}
	}

	// 3. AnimInstance에 타겟 전달 (머리 추적용)
	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (Dragon && Dragon->GetMesh())
	{
		// TODO: Set GameplayTag on AvatarActor for head tracking
		// AnimBP will use Layered Blend Per Bone to track PrimaryTarget
		// Example: Dragon->GetAbilitySystemComponent()->AddLooseGameplayTag(Tag_State_Dragon_Breathing);
	}
	
	if (CurrentActorInfo->IsNetAuthority())
	{
		OnDamageReceivedHandle = GetAbilitySystemComponentFromActorInfo()
			->OnGameplayEffectAppliedDelegateToSelf.AddUObject(
				this,
				&USFGA_Dragon_FlameBreath_Line::OnDamageReceivedDuringCharge
			);
	}
	
	GetWorld()->GetTimerManager().SetTimer(
		ChargeTimerHandle,
		this,
		&USFGA_Dragon_FlameBreath_Line::TransitionToBreath,
		ChargeDuration,
		false
	);
	//  UI 표시
	// TODO: GameplayCue로 경고 UI 표시
}

void USFGA_Dragon_FlameBreath_Line::TransitionToBreath()
{
	// TODO: Remove Charging tag, Add Breathing tag
	
	if (OnDamageReceivedHandle.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->OnGameplayEffectAppliedDelegateToSelf.Remove(OnDamageReceivedHandle);
		OnDamageReceivedHandle.Reset();
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

	// 이건 데미지 주는 거 매틱마다 
	GetWorld()->GetTimerManager().SetTimer(
		BreathTickTimer,
		this,
		&USFGA_Dragon_FlameBreath_Line::ApplyBreathDamage,
		BreathTickRate,
		true  
	);
	// 이건 Breath 타이머 
	GetWorld()->GetTimerManager().SetTimer(
		BreathDurationTimer,
		this,
		&USFGA_Dragon_FlameBreath_Line::StopBreath,
		BreathDuration,
		false
	);
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
	// TODO: Remove Breathing tag
	// bIsBreathing = false;
	
	GetWorld()->GetTimerManager().ClearTimer(BreathTickTimer);

	
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
			FName("BreathEnd"),  // BreathEnd 섹션 감지
			false  
		);

		if (BreathEndMontageTask)
		{
			// OnCompleted: BreathEnd 섹션이 자연스럽게 끝나면 호출됨
			BreathEndMontageTask->OnCompleted.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnBreathEndCompleted);
			BreathEndMontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnMontageInterrupted);
			BreathEndMontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnMontageCancelled);
			BreathEndMontageTask->ReadyForActivation();
		}
	}
}


AActor* USFGA_Dragon_FlameBreath_Line::FindPrimaryTarget()
{
	ISFAIControllerInterface* AIC = Cast<ISFAIControllerInterface>(GetControllerFromActorInfo()->GetPawn()->GetController());
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


void USFGA_Dragon_FlameBreath_Line::OnDamageReceivedDuringCharge( UAbilitySystemComponent* Source,  const FGameplayEffectSpec& SpecApplied,  FActiveGameplayEffectHandle ActiveHandle)
{
	// TODO: Check GameplayTag instead of bool
	// if (!Dragon->HasMatchingGameplayTag(Tag_State_Dragon_Charging)) return;

	
	float Damage = SpecApplied.GetSetByCallerMagnitude(SFGameplayTags::Data_Damage_BaseDamage,false, 0.f );

	if (Damage > 0.f)
	{
		AccumulatedInterruptDamage += Damage;

		// 중단 성공
		if (AccumulatedInterruptDamage >= InterruptThreshold)
		{
			InterruptBreath();
		}
	}
}
void USFGA_Dragon_FlameBreath_Line::InterruptBreath()
{
	
	GetWorld()->GetTimerManager().ClearTimer(ChargeTimerHandle);
	
	if (InterruptStaggerEffect)
	{
		UAbilitySystemComponent* DragonASC = GetAbilitySystemComponentFromActorInfo();
		if (DragonASC)
		{
			FGameplayEffectSpecHandle StaggerSpec =
				MakeOutgoingGameplayEffectSpec(InterruptStaggerEffect, 1);

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
	
	GetWorld()->GetTimerManager().ClearTimer(ChargeTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(BreathTickTimer);
	GetWorld()->GetTimerManager().ClearTimer(BreathDurationTimer);
	
	if (BreathEndMontageTask)
	{
		BreathEndMontageTask->EndTask();
		BreathEndMontageTask = nullptr;
	}
	
	if (OnDamageReceivedHandle.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()
			->OnGameplayEffectAppliedDelegateToSelf.Remove(OnDamageReceivedHandle);
		OnDamageReceivedHandle.Reset();
	}
	
	// TODO: Remove all breath-related tags from AvatarActor


	// 5. 상태 초기화
	PrimaryTarget.Reset();
	HitActors.Empty();
	AccumulatedInterruptDamage = 0.f;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}