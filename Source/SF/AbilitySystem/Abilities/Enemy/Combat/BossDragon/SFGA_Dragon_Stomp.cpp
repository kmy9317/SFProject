// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGA_Dragon_Stomp.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GameplayEffect/SFGameplayEffectContext.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterBase.h"
#include "Engine/OverlapResult.h"

USFGA_Dragon_Stomp::USFGA_Dragon_Stomp()
{
	
}

void USFGA_Dragon_Stomp::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                         const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                         const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (StompMontage)
	{

		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			StompMontage,
			1.f,
			NAME_None,
			true
		);


		if (MontageTask)
		{
			MontageTask->OnCompleted.AddDynamic(this, &USFGA_Dragon_Stomp::OnMontageCompleted);
			MontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_Stomp::OnMontageInterrupted);
			MontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_Stomp::OnMontageCancelled);
			MontageTask->ReadyForActivation();
		}
	}
	
	if (!ActorInfo->IsNetAuthority())
	{
		return;
	}
	UAbilityTask_WaitGameplayEvent* WaitEventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			SFGameplayTags::GameplayEvent_Tracing,
			nullptr,
			true,
			true
			);

	if (WaitEventTask)
	{
		WaitEventTask->EventReceived.AddDynamic(this, &USFGA_Dragon_Stomp::EmitShockWave);
		WaitEventTask->ReadyForActivation();
	}
	
}


void USFGA_Dragon_Stomp::EmitShockWave(FGameplayEventData Payload)
{
	ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
	if (!Dragon)
	{
		return;
	}

	// HitResult Null 체크
	const FHitResult* HitResult = Payload.ContextHandle.GetHitResult();
	if (!HitResult)
	{
		UE_LOG(LogTemp, Warning, TEXT("EmitShockWave: No HitResult in Payload!"));
		return;
	}

	FVector StompLoc = HitResult->Location;

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetAvatarActorFromActorInfo());

	bool bHit = Dragon->GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		StompLoc,
		FQuat::Identity,
		ECollisionChannel::ECC_Pawn,
		FCollisionShape::MakeSphere(ShockwaveRadius),
		QueryParams
	);

	// 디버그 표시
	if (bIsDebug)
	{
		DrawDebugSphere(
			Dragon->GetWorld(),
			StompLoc,
			ShockwaveRadius,
			32,
			FColor::Red,
			false,
			3.0f,
			0,
			2.0f
		);
	}

	if (bHit)
	{

		FGameplayEffectContextHandle EffectContext =
			MakeEffectContext(CurrentSpecHandle, CurrentActorInfo);

		for (const FOverlapResult& Overlap : OverlapResults)
		{
			AActor* HitActor = Overlap.GetActor();

			if (HitActor && GetAttitudeTowards(HitActor) == ETeamAttitude::Hostile)
			{
				ApplyDamageToTarget(HitActor, EffectContext);
				ApplyKnockBackToTarget(HitActor, StompLoc);

				// 디버그: 히트된 액터 표시
				if (bIsDebug)
				{
					DrawDebugLine(
						Dragon->GetWorld(),
						StompLoc,
						HitActor->GetActorLocation(),
						FColor::Green,
						false,
						3.0f,
						0,
						2.0f
					);
				}
			}
		}
	}
}

void USFGA_Dragon_Stomp::ApplyKnockBackToTarget(AActor* Target, const FVector& StompLocation)
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
	
    FVector KnockBackDirection = FVector::UpVector;
    
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

void USFGA_Dragon_Stomp::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Dragon_Stomp::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Dragon_Stomp::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

