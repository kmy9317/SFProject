#include "SFGA_Interact_Active.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_NetworkSyncPoint.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/Tasks/Interaction/SFAbilityTask_WaitForInvalidInteraction.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/SFCharacterBase.h"
#include "Equipment/EquipmentComponent/SFEquipmentComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Messages/SFInteractionMessages.h"
#include "Messages/SFMessageGameplayTags.h"

USFGA_Interact_Active::USFGA_Interact_Active(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = ESFAbilityActivationPolicy::Manual;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 클라이언트의 취소 요청을 서버가 존중 (입력 해제 시 즉시 취소)
	bServerRespectsRemoteAbilityCancellation = true;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 서버에서만 종료 가능
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnlyTermination;
	AbilityTags.AddTag(SFGameplayTags::Ability_Interact_Active);
	ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_Interact);

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		// 게임플레이 이벤트로 트리거되도록 설정
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = SFGameplayTags::Ability_Interact_Active;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void USFGA_Interact_Active::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (TriggerEventData == nullptr)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	if (InitializeAbility(const_cast<AActor*>(TriggerEventData->Target.Get())) == false)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	// 상호작용 엑터에 홀딩 시작 알림
	NotifyInteractableActiveStarted();

	// InteractionType에 따른 분기
	switch (InteractionInfo.InteractionType)
	{
	case ESFInteractionType::Instant:
		TriggerInteraction();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	case ESFInteractionType::TimedHold:
		StartHoldingInteraction();
		SendProgressMessage();
		GetWorld()->GetTimerManager().SetTimer(
			HoldingTimerHandle, 
			this, 
			&ThisClass::OnDurationEnded, 
			InteractionInfo.Duration, 
			false
		);
		break;
	case ESFInteractionType::GaugeBased:
		StartHoldingInteraction();
		SendInteractingMessage();
		WaitForGaugeBasedComplete();
		break;
	}
}

void USFGA_Interact_Active::OnInvalidInteraction()
{
	// 홀딩 즉시 취소 (위치/각도 이탈로 인한 취소)
	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

void USFGA_Interact_Active::StartHoldingInteraction()
{
	FlushPressedInput(MoveInputAction);
	
	if (ASFCharacterBase* SFCharacter = GetSFCharacterFromActorInfo())
	{
		if (UCharacterMovementComponent* CharacterMovement = SFCharacter->GetCharacterMovement())
		{
			CharacterMovement->StopMovementImmediately();
		}

		if (USFEquipmentComponent* EquipmentComponent = SFCharacter->GetComponentByClass<USFEquipmentComponent>())
		{
			EquipmentComponent->HideWeapons();
		}
	}

	FGameplayCueParameters Parameters;
	Parameters.Instigator = InteractableActor;
	K2_AddGameplayCueWithParams(InteractionInfo.ActiveLoopGameplayCueTag, Parameters, true);

	FSFMontagePlayData MontageData = GetInteractionStartMontage();
	if (MontageData.IsValid() && MontageData.Montage)
	{
		if (UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("InteractMontage"), MontageData.Montage, MontageData.PlayRate, MontageData.StartSection, true, 1.f, 0.f, false))
		{
			PlayMontageTask->ReadyForActivation();
		}
	}

	if (USFAbilityTask_WaitForInvalidInteraction* InvalidInteractionTask = USFAbilityTask_WaitForInvalidInteraction::WaitForInvalidInteraction(this, AcceptanceAngle, AcceptanceDistance))
	{
		InvalidInteractionTask->OnInvalidInteraction.AddDynamic(this, &ThisClass::OnInvalidInteraction);
		InvalidInteractionTask->ReadyForActivation();
	}

	if (UAbilityTask_WaitInputRelease* InputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, false))
	{
		InputReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnInputReleased);
		InputReleaseTask->ReadyForActivation();
	}
}

void USFGA_Interact_Active::SendProgressMessage()
{
	if (IsLocallyControlled())
	{
		if (UGameplayMessageSubsystem::HasInstance(this))
		{
			FSFInteractionMessage Message;
			Message.Instigator = GetAvatarActorFromActorInfo();
			Message.bShouldRefresh = true;
			Message.bSwitchActive = true;
			Message.InteractionInfo = InteractionInfo;
			UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
			MessageSubsystem.BroadcastMessage(SFGameplayTags::Message_Interaction_Progress, Message);
		}
	}
}

void USFGA_Interact_Active::SendInteractingMessage()
{
	if (IsLocallyControlled())
	{
		if (UGameplayMessageSubsystem::HasInstance(this))
		{
			FSFInteractionMessage Message;
			Message.Instigator = GetAvatarActorFromActorInfo();
			Message.bShouldRefresh = true;
			Message.bSwitchActive = true;
			Message.InteractionInfo = InteractionInfo;
			UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
			MessageSubsystem.BroadcastMessage(SFGameplayTags::Message_Interaction_Interacting, Message);
		}
	}
}

void USFGA_Interact_Active::NotifyInteractableActiveStarted()
{
	if (Interactable)
	{
		Interactable->OnInteractActiveStarted(GetSFCharacterFromActorInfo());
	}
}

void USFGA_Interact_Active::NotifyInteractableActiveEnded()
{
	if (Interactable)
	{
		Interactable->OnInteractActiveEnded(GetSFCharacterFromActorInfo());
	}
}

void USFGA_Interact_Active::OnInputReleased(float TimeHeld)
{
	// 홀딩 즉시 취소 (플레이어가 키를 뗌)
	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

void USFGA_Interact_Active::OnDurationEnded()
{
	// 멀티플레이어 환경에서 서버와 클라이언트 간 타이밍 동기화
	// 서버만 대기하고 클라이언트는 즉시 진행 (서버 권한 보장)
	if (UAbilityTask_NetworkSyncPoint* NetSyncTask = UAbilityTask_NetworkSyncPoint::WaitNetSync(this, EAbilityTaskNetSyncType::OnlyServerWait))
	{
		NetSyncTask->OnSync.AddDynamic(this, &ThisClass::OnNetSync);
		NetSyncTask->ReadyForActivation();
	}
}

void USFGA_Interact_Active::OnNetSync()
{
	// 실제 상호작용 트리거 시도
	if (TriggerInteraction())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
	else
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
}

bool USFGA_Interact_Active::TriggerInteraction()
{
	bool bTriggerSuccessful = false;
	bool bCanActivate = false;

	FGameplayEventData Payload;
	Payload.EventTag = SFGameplayTags::Ability_Interact;  
	Payload.Instigator = GetAvatarActorFromActorInfo(); // 상호작용 요청자 (플레이어)
	Payload.Target = InteractableActor; // 상호작용 대상 액터

	// 상호작용 대상이 이벤트 데이터를 커스터마이징할 기회 제공
	Interactable->CustomizeInteractionEventData(SFGameplayTags::Ability_Interact, Payload);

	if (UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo())
	{
		if (FGameplayAbilitySpec* AbilitySpec = AbilitySystem->FindAbilitySpecFromClass(InteractionInfo.AbilityToGrant))
		{
			// 어빌리티 활성화 가능 여부 확인
			bCanActivate = AbilitySpec->Ability->CanActivateAbility(AbilitySpec->Handle, AbilitySystem->AbilityActorInfo.Get());
			bTriggerSuccessful = AbilitySystem->TriggerAbilityFromGameplayEvent(
				AbilitySpec->Handle,
				AbilitySystem->AbilityActorInfo.Get(),
				SFGameplayTags::Ability_Interact,
				&Payload,
				*AbilitySystem
			);
		}
	}

	return bCanActivate || bTriggerSuccessful;
}

void USFGA_Interact_Active::WaitForGaugeBasedComplete()
{
	if (!InteractionInfo.GaugeBasedCompleteEventTag.IsValid())
	{
		// 완료 이벤트 태그가 없으면 취소
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	if (UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, InteractionInfo.GaugeBasedCompleteEventTag))
	{
		WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnGaugeBasedCompleted);
		WaitEventTask->ReadyForActivation();
	}
}

void USFGA_Interact_Active::OnGaugeBasedCompleted(FGameplayEventData Payload)
{
	if (TriggerInteraction())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
	else
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
}

FSFMontagePlayData USFGA_Interact_Active::GetInteractionStartMontage() const
{
	if (InteractionInfo.ActiveStartMontageTag.IsValid())
	{
		if (const USFHeroAnimationData* AnimData = GetHeroAnimationData())
		{
			return AnimData->GetSingleMontage(InteractionInfo.ActiveStartMontageTag);
		}
	}

	return FSFMontagePlayData();
}

void USFGA_Interact_Active::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (HoldingTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(HoldingTimerHandle);
	}
	
	if (ASFCharacterBase* SFCharacter = GetSFCharacterFromActorInfo())
	{
		if (bWasCancelled)
		{
			if (USFEquipmentComponent* EquipmentComp = GetEquipmentComponent())
			{
				EquipmentComp->ShowWeapons();
				FSFMontagePlayData MontageData = GetMainHandEquipMontageData();
				ExecuteMontageGameplayCue(MontageData);
			}
		}
		
		// 상호작용 대상에 홀딩 종료 알림
		NotifyInteractableActiveEnded();

		if (IsLocallyControlled() && UGameplayMessageSubsystem::HasInstance(this))
		{
			// UI에 홀딩 종료 알림 (진행률 바 숨김 등)
			FSFInteractionMessage Message;
			Message.Instigator = SFCharacter;
			Message.bShouldRefresh = false;
			Message.bSwitchActive = true; // 홀딩 상태 해제
			UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
			MessageSubsystem.BroadcastMessage(SFGameplayTags::Message_Interaction_Notice, Message);
		}
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

