// SFGA_Enemy_Melee.cpp
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
	Penetration(1),
	MontageTask(nullptr),
	EventTask(nullptr)
{
}

void USFGA_Enemy_Melee::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!AttackTypeMontage.AnimMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		AttackTypeMontage.AnimMontage);

	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &USFGA_Enemy_Melee::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &USFGA_Enemy_Melee::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &USFGA_Enemy_Melee::OnMontageCancelled);
		MontageTask->ReadyForActivation();
	}

	if (!ActorInfo->IsNetAuthority())
		return;

	EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		SFGameplayTags::GameplayEvent_TraceHit);

	if (EventTask)
	{
		EventTask->EventReceived.AddDynamic(this, &USFGA_Enemy_Melee::OnTraceHit);
		EventTask->ReadyForActivation();
	}
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

	AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
	if (!IsValid(HitActor)) return;
	
	ASFCharacterBase* HitCharacter = Cast<ASFCharacterBase>(HitActor);
	if (!HitCharacter) return;

	if (GetAttitudeTowards(HitActor) != ETeamAttitude::Hostile)
	{
		return;
	}
	
	FGameplayEffectContextHandle Context = Payload.ContextHandle;
	{
		ApplyDamageToTarget(HitActor, Context);
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

	if (bWasCancelled && AttackTypeMontage.AnimMontage)
	{
		if (UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance())
		{
			if (AnimInstance->Montage_IsPlaying(AttackTypeMontage.AnimMontage))
			{
				AnimInstance->Montage_Stop(0.0f, AttackTypeMontage.AnimMontage);
			}
		}
	}
	
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}
	
	if (EventTask)
	{
		EventTask->EndTask();
		EventTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}