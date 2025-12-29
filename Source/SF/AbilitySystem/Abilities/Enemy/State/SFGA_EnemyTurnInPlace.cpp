#include "SFGA_EnemyTurnInPlace.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterGameplayTags.h"
#include "AI/Controller/SFBaseAIController.h"
#include "AI/Controller/SFTurnInPlaceComponent.h"
#include "GameFramework/Character.h"

USFGA_EnemyTurnInPlace::USFGA_EnemyTurnInPlace()
{
	ActivationPolicy = ESFAbilityActivationPolicy::Manual;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	AbilityTags.AddTag(SFGameplayTags::Ability_Enemy_TurnInPlace);

	ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_TurningInPlace);
	ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_UsingAbility);

	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Dead);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Stunned);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_Knockdown);
	ActivationBlockedTags.AddTag(SFGameplayTags::Character_State_TurningInPlace);

	MontagePlayRate = 1.0f;
}

void USFGA_EnemyTurnInPlace::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ValidateTriggerEvent(TriggerEventData))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TriggerEventTag = TriggerEventData->EventTag;
	ActualTurnYaw = TriggerEventData->EventMagnitude;

	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		float CurrentYaw = Avatar->GetActorRotation().Yaw;
		TargetYaw = CurrentYaw + ActualTurnYaw;
	}

	if (!StartTurnMontage(TriggerEventTag))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ASFBaseAIController* AI = Cast<ASFBaseAIController>(ActorInfo->PlayerController.Get()))
	{
		AI->DisableTurnInPlaceFor(2.0f);
	}
}

void USFGA_EnemyTurnInPlace::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{

	CleanupMontageTask();
	ActualTurnYaw = 0.f;
	TriggerEventTag = FGameplayTag::EmptyTag;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGA_EnemyTurnInPlace::OnTurnComplete()
{
	if (ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (ASFBaseAIController* AI = Cast<ASFBaseAIController>(Char->GetController()))
		{
			if (USFTurnInPlaceComponent* TurnComp = AI->GetTurnInPlaceComponent())
			{
				TurnComp->OnTurnFinished();
			}
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_EnemyTurnInPlace::OnTurnCancelled()
{
	NotifyTurnFinished();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_EnemyTurnInPlace::OnTurnInterrupted()
{
	NotifyTurnFinished();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

bool USFGA_EnemyTurnInPlace::ValidateTriggerEvent(const FGameplayEventData* TriggerEventData)
{
	if (!TriggerEventData || !TriggerEventData->EventTag.IsValid())
	{
		return false;
	}

	const FGameplayTag TurnParentTag =
		FGameplayTag::RequestGameplayTag(TEXT("GameplayEvent.Turn"));

	return TriggerEventData->EventTag.MatchesTag(TurnParentTag);
}

bool USFGA_EnemyTurnInPlace::StartTurnMontage(const FGameplayTag& EventTag)
{
	UAnimMontage* const* MontagePtr = TurnMontageMap.Find(EventTag);
	if (!MontagePtr || !*MontagePtr)
	{
		return false;
	}

	CurrentMontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			*MontagePtr,
			MontagePlayRate,
			NAME_None,
			true
		);

	if (!CurrentMontageTask)
	{
		return false;
	}

	CurrentMontageTask->OnCompleted.AddDynamic(this, &USFGA_EnemyTurnInPlace::OnTurnComplete);
	CurrentMontageTask->OnCancelled.AddDynamic(this, &USFGA_EnemyTurnInPlace::OnTurnCancelled);
	CurrentMontageTask->OnInterrupted.AddDynamic(this, &USFGA_EnemyTurnInPlace::OnTurnInterrupted);

	CurrentMontageTask->ReadyForActivation();
	return true;
}

void USFGA_EnemyTurnInPlace::CleanupMontageTask()
{
	if (CurrentMontageTask)
	{
		CurrentMontageTask->EndTask();
		CurrentMontageTask = nullptr;
	}
}

void USFGA_EnemyTurnInPlace::NotifyTurnFinished()
{
	if (ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (ASFBaseAIController* AI = Cast<ASFBaseAIController>(Char->GetController()))
		{
			if (USFTurnInPlaceComponent* TurnComp = AI->GetTurnInPlaceComponent())
			{
				TurnComp->OnTurnFinished();
			}
		}
	}
}
