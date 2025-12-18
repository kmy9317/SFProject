// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGA_Dragon_TailSwipe.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GameplayEffect/SFGameplayEffectContext.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterBase.h"
#include "DrawDebugHelpers.h"

USFGA_Dragon_TailSwipe::USFGA_Dragon_TailSwipe()
{
}

void USFGA_Dragon_TailSwipe::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                             const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Montage 재생
	if (TailSwipeMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			TailSwipeMontage,
			1.f,
			NAME_None,
			true
		);

		if (MontageTask)
		{
			MontageTask->OnCompleted.AddDynamic(this, &USFGA_Dragon_TailSwipe::OnMontageCompleted);
			MontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_TailSwipe::OnMontageInterrupted);
			MontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_TailSwipe::OnMontageCancelled);
			MontageTask->ReadyForActivation();
		}
	}

	// Authority에서만 히트 이벤트 대기
	if (!ActorInfo->IsNetAuthority())
	{
		return;
	}

	// AnimNotifyState_SweepTrace에서 보내는 GameplayEvent 대기
	// OnlyTriggerOnce = false로 설정하여 여러 번 히트 가능
	WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		SFGameplayTags::GameplayEvent_Tracing,
		nullptr,
		false,  // OnlyTriggerOnce = false (여러 번 트리거 가능)
		true
	);

	if (WaitEventTask)
	{
		WaitEventTask->EventReceived.AddDynamic(this, &USFGA_Dragon_TailSwipe::OnTailHit);
		WaitEventTask->ReadyForActivation();
	}
}



void USFGA_Dragon_TailSwipe::OnTailHit(FGameplayEventData Payload)
{
	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (!Dragon)
	{
		return;
	}


	const FHitResult* HitResult = Payload.ContextHandle.GetHitResult();
	if (!HitResult)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnTailHit: No HitResult in Payload!"));
		return;
	}

	AActor* HitActor = HitResult->GetActor();
	if (!HitActor)
	{
		return;
	}
	
	if (GetAttitudeTowards(HitActor) != ETeamAttitude::Hostile)
	{
		return;
	}
	
	FGameplayEffectContextHandle EffectContext = MakeEffectContext(CurrentSpecHandle, CurrentActorInfo);
	EffectContext.AddHitResult(*HitResult);

	ApplyDamageToTarget(HitActor, EffectContext);
	
	ApplyKnockBackToTarget(HitActor, HitResult->ImpactPoint);
}

void USFGA_Dragon_TailSwipe::ApplyKnockBackToTarget(AActor* Target, const FVector& HitLocation)
{
	if (!Target)
	{
		return;
	}

	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (!Dragon)
	{
		return;
	}

	// 꼬리 휘두르는 방향으로 넉백 (Dragon의 오른쪽 방향)
	FVector KnockBackDirection = Dragon->GetActorRightVector();

	// GameplayEvent 데이터 생성
	FGameplayEventData EventData;
	EventData.Instigator = Dragon;
	EventData.Target = Target;
	EventData.EventMagnitude = 1.0f; // 넉백 강도 배율

	// KnockBack 방향을 ContextHandle에 저장
	FHitResult HitResult;
	HitResult.ImpactPoint = Target->GetActorLocation();
	HitResult.ImpactNormal = KnockBackDirection;
	EventData.ContextHandle.AddHitResult(HitResult, true);

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

	if (TargetASC)
	{
		TargetASC->HandleGameplayEvent(
			SFGameplayTags::GameplayEvent_Knockback,
			&EventData
		);
	}
}

void USFGA_Dragon_TailSwipe::OnMontageCompleted()
{
	
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Dragon_TailSwipe::OnMontageInterrupted()
{
	
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Dragon_TailSwipe::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Dragon_TailSwipe::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (WaitEventTask)
	{
		WaitEventTask->EndTask();
		WaitEventTask = nullptr;
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
