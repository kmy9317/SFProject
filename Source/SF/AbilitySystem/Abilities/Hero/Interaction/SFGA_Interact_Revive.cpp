#include "SFGA_Interact_Revive.h"

#include "AbilitySystemComponent.h"
#include "Player/Components/SFPlayerCombatStateComponent.h"

USFGA_Interact_Revive::USFGA_Interact_Revive(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

void USFGA_Interact_Revive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!bInitialized)
	{
		return;
	}

	if (!HasAuthority(&ActivationInfo))
	{
		return;
	}

	// 부활 횟수 증가
	if (USFPlayerCombatStateComponent* CombatState = USFPlayerCombatStateComponent::FindPlayerCombatStateComponent(GetAvatarActorFromActorInfo()))
	{
		CombatState->IncrementReviveCount();
	}

	// TODO : 부활 보상 GE 적용
	if (ReviveRewardEffect)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(ReviveRewardEffect, GetAbilityLevel());
			if (SpecHandle.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}

	// TODO : 부활 성공 GameplayCue
	if (ReviveSuccessGameplayCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Instigator = GetAvatarActorFromActorInfo();
		CueParams.EffectCauser = InteractableActor;
		K2_ExecuteGameplayCueWithParams(ReviveSuccessGameplayCueTag, CueParams);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}