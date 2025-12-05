// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_Melee.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AI/SFAIGameplayTags.h"
#include "Character/SFCharacterBase.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "Equipment/EquipmentInstance/SFEquipmentInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interface/SFTraceActorInterface.h"

// <-- 추가된 include: 커스텀 EffectContext
#include "AbilitySystem/GameplayEffect/Enemy/EffectContext/FSFHitEffectContext.h"

class USFEquipmentComponent;

USFGA_Enemy_Melee::USFGA_Enemy_Melee(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	Penetration(1)
{
}

void USFGA_Enemy_Melee::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 기본 검증
	if (AttackTypeMontage.AnimMontage == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 몽타주 재생 
	UAbilityTask_PlayMontageAndWait* PlayMontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			AttackTypeMontage.AnimMontage,
			1.0f,
			NAME_None,
			true,
			1.0f,
			0.0f,
			false);
	
	if (PlayMontageTask)
	{
		PlayMontageTask->OnCompleted.AddDynamic(this, &USFGA_Enemy_Melee::OnMontageCompleted);
		PlayMontageTask->OnInterrupted.AddDynamic(this, &USFGA_Enemy_Melee::OnMontageInterrupted);
		PlayMontageTask->OnCancelled.AddDynamic(this, &USFGA_Enemy_Melee::OnMontageCancelled);
		PlayMontageTask->ReadyForActivation();
	}

	// 서버에서만 게임플레이 로직 실행
	if (!ActorInfo->IsNetAuthority())
	{
		return;
	}

	// --- 이하 서버 전용 로직 ---
	
	CurrentPenetration = Penetration;
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 트레이스 히트 이벤트 대기 (서버만)
	UAbilityTask_WaitGameplayEvent* WaitGameplayEventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			SFGameplayTags::GameplayEvent_TraceHit,
			nullptr,
			false,
			false);
	WaitGameplayEventTask->EventReceived.AddDynamic(this, &USFGA_Enemy_Melee::OnTraceHit);
	WaitGameplayEventTask->ReadyForActivation();
}

void USFGA_Enemy_Melee::CleanupWeaponTraces()
{
	// 서버에서만 실행되어야 함
	if (!GetAvatarActorFromActorInfo()->HasAuthority())
	{
		return;
	}

	AActor* Owner = GetAvatarActorFromActorInfo();
	if (!Owner)
		return;
    
	USFEquipmentComponent* EquipmentComp = USFEquipmentComponent::FindEquipmentComponent(Owner);
	if (!EquipmentComp)
		return;
    
	const TArray<USFEquipmentInstance*>& EquippedItems = EquipmentComp->GetEquippedItems();
	for (USFEquipmentInstance* Instance : EquippedItems)
	{
		if (!Instance)
			continue;
        
		const TArray<AActor*>& SpawnedActors = Instance->GetSpawnedActors();
		for (AActor* Actor : SpawnedActors)
		{
			if (ISFTraceActorInterface* TraceActor = Cast<ISFTraceActorInterface>(Actor))
			{
				TraceActor->OnTraceEnd(Owner);
			}
		}
	}
}

void USFGA_Enemy_Melee::OnTraceHit(FGameplayEventData Payload)
{
	if (!GetAvatarActorFromActorInfo()->HasAuthority())
		return;

	const FGameplayAbilityTargetData* RawData = Payload.TargetData.Get(0);
	const FGameplayAbilityTargetData_SingleTargetHit* HitData =
		static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(RawData);

	if (!HitData)
		return;

	const FHitResult& Hit = HitData->HitResult;

	// 기본값 (폴백)
	FVector AttackDirection = FVector::ZeroVector;
	FVector AttackLocation  = FVector::ZeroVector;

	// ========== 1) 먼저 Payload의 Context에서 FSFHitEffectContext 읽기 ==========
	if (const FSFHitEffectContext* SFContext = static_cast<const FSFHitEffectContext*>(Payload.ContextHandle.Get()))
	{
		AttackDirection = SFContext->GetAttackDirection();
		AttackLocation  = SFContext->GetHitLocation();
	}

	// ========== 2) Context에 정보가 없으면 HitResult 기반 폴백 ==========
	if (AttackLocation.IsNearlyZero())
	{
		AttackLocation = Hit.ImpactPoint;
	}

	if (AttackDirection.IsNearlyZero())
	{
		// Hit.ImpactNormal에 값이 들어있을 수 있음 
		if (!Hit.ImpactNormal.IsNearlyZero())
		{
			AttackDirection = Hit.ImpactNormal.GetSafeNormal();
		}
		else
		{
			// Instigator(공격자) 위치 기준으로 계산 (마지막 수단)
			if (AActor* Instigator = const_cast<AActor*>(Payload.Instigator.Get()))
			{
				AttackDirection = (Hit.ImpactPoint - Instigator->GetActorLocation()).GetSafeNormal();
			}
		}
	}

	AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
	if (!IsValid(HitActor)) return;

	ASFCharacterBase* HitCharacter = Cast<ASFCharacterBase>(HitActor);
	if (!HitCharacter) return;

	// 패링 체크
	if (HitCharacter->HasMatchingGameplayTag(SFGameplayTags::Character_State_Parrying))
	{
		ApplyParriedEffectToSelf();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 관통 처리
	if (CurrentPenetration > 0)
	{
		ApplyDamageToTarget(HitActor, AttackDirection, AttackLocation);
		CurrentPenetration--;
	}
}


void USFGA_Enemy_Melee::ApplyParriedEffectToSelf()
{
	if (!ParriedGameplayEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (!OwnerASC)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = OwnerASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle EffectHandle = OwnerASC->MakeOutgoingSpec(
		ParriedGameplayEffectClass,
		GetAbilityLevel(),
		EffectContext);
		
	if (EffectHandle.IsValid())
	{
		OwnerASC->ApplyGameplayEffectSpecToSelf(*EffectHandle.Data.Get());	
	}
}

void USFGA_Enemy_Melee::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Enemy_Melee::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Enemy_Melee::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Enemy_Melee::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	CleanupWeaponTraces();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
