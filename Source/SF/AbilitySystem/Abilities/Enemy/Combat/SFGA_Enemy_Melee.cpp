// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_Melee.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AI/SFAIGameplayTags.h"
#include "Character/SFCharacterBase.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "Equipment/EquipmentInstance/SFEquipmentInstance.h"
#include "Interface/SFTraceActorInterface.h"

class USFEquipmentComponent;

USFGA_Enemy_Melee::USFGA_Enemy_Melee(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	Penetration(1)
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(SFGameplayTags::Ability_BaseAttack_Melee);
	SetAssetTags(AssetTags);
}

void USFGA_Enemy_Melee::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, 
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CurrentPenetration = Penetration;
	// 1. 기본 검증
	if (AttackTypeMontage.AnimMontage == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 서버에서 Cost/Cooldown 검증
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. Client 예측 
	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
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
}


void USFGA_Enemy_Melee::EndAbility(	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{

	CleanupWeaponTraces();
    
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGA_Enemy_Melee::CleanupWeaponTraces() // 만약 도중에 죽을 수도 있으니 무기 트레이스 꺼버리기 
{
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
	if (CurrentPenetration > 0)
	{
		ApplyDamageToTarget(HitActor, BaseDamage);
		CurrentPenetration--;
	}
	if (HitCharacter->HasMatchingGameplayTag(SFGameplayTags::Character_State_Parrying))
	{
		// TODO: 경직 상태로 전환하거나 패링 리액션 재생
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	

	
	return;
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