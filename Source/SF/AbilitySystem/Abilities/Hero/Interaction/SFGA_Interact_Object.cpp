#include "SFGA_Interact_Object.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/Tasks/Interaction/SFAbilityTask_WaitForInvalidInteraction.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "Interaction/SFInteractable.h"
#include "Interaction/SFInteractionQuery.h"

USFGA_Interact_Object::USFGA_Interact_Object(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = ESFAbilityActivationPolicy::Manual;
	
	AbilityTags.AddTag(SFGameplayTags::Ability_Interact_Object);
	ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_Interact);

	bShouldPersistOnTravel = false;
}

void USFGA_Interact_Object::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (TriggerEventData == nullptr)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	// 상호작용 대상 초기화 
	AActor* TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
	if (InitializeAbility(TargetActor) == false)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	// 초기화 상태 리셋
	bInitialized = false;

	FSFInteractionQuery Query;
	Query.RequestingAvatar = GetAvatarActorFromActorInfo();
	Query.RequestingController = GetControllerFromActorInfo();

	// 상호작용 가능 여부 확인
	if (Interactable->CanInteraction(Query) == false)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	if (CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo) == false)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	// 초기화 완료 표시
	bInitialized = true;

	if (ISFInteractable* InteractableInterface = Cast<ISFInteractable>(InteractableActor))
	{
		InteractableInterface->OnInteractionSuccess(GetAvatarActorFromActorInfo());
	}

	// 상호작용 중 플레이어가 범위를 벗어나면 취소
	if (USFAbilityTask_WaitForInvalidInteraction* InvalidInteractionTask = USFAbilityTask_WaitForInvalidInteraction::WaitForInvalidInteraction(this, AcceptanceAngle, AcceptanceDistance))
	{
		InvalidInteractionTask->OnInvalidInteraction.AddDynamic(this, &ThisClass::OnInvalidInteraction);
		InvalidInteractionTask->ReadyForActivation();
	}

	// 상호작용 대상에 대해 종료전 애니메이션 몽타주 재생
	FSFMontagePlayData MontageData = GetInteractionEndMontage();
	bHasEndMontage = MontageData.IsValid() && MontageData.Montage;

	if (bHasEndMontage)
	{
		if (USFEquipmentComponent* EquipmentComp = GetEquipmentComponent())
		{
			EquipmentComp->HideWeapons();
		}
		if (UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("InteractMontage"), MontageData.Montage, MontageData.PlayRate, MontageData.StartSection, true, 1.f, 0.f, false))
		{
			PlayMontageTask->ReadyForActivation();
		}
	}
}

void USFGA_Interact_Object::OnInvalidInteraction()
{
	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

FSFMontagePlayData USFGA_Interact_Object::GetInteractionEndMontage() const
{
	if (InteractionInfo.ActiveEndMontageTag.IsValid())
	{
		if (const USFHeroAnimationData* AnimData = GetHeroAnimationData())
		{
			return AnimData->GetSingleMontage(InteractionInfo.ActiveEndMontageTag);
		}
	}

	return FSFMontagePlayData();
}

void USFGA_Interact_Object::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (USFEquipmentComponent* EquipmentComp = GetEquipmentComponent())
	{
		EquipmentComp->ShowWeapons();
	}
	
	if (bHasEndMontage)
	{
		if (USFEquipmentComponent* EquipmentComp = GetEquipmentComponent())
		{
			FSFMontagePlayData MontageData = GetMainHandEquipMontageData();
			ExecuteMontageGameplayCue(MontageData);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

