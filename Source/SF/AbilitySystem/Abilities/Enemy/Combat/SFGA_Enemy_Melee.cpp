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

	// 기본 검증 (모든 머신)
	if (AttackTypeMontage.AnimMontage == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 몽타주 재생 (모든 머신에서 실행 - 클라이언트도 애니메이션 보임)
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
	// 서버 체크 (이미 서버 전용 Task지만 안전장치)
	if (!GetAvatarActorFromActorInfo()->HasAuthority())
	{
		return;
	}

	AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
	if (!IsValid(HitActor))
	{
		return;
	}

	ASFCharacterBase* HitCharacter = Cast<ASFCharacterBase>(HitActor);
	if (!HitCharacter)
	{
		return;
	}
	
	// 패링 체크
	if (HitCharacter->HasMatchingGameplayTag(SFGameplayTags::Character_State_Parrying))
	{
		ApplyParriedEffectToSelf();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	
	// 관통 공격 처리
	if (CurrentPenetration > 0)
	{
		ApplyDamageToTarget(HitActor);
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