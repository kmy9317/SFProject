// Fill out your copyright notice in the Description page of Project Settings.


#include "SFOverlayWidgetController.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Hero/SFPrimarySet_Hero.h"
#include "Interface/SFChainedSkill.h"
#include "Messages/SFMessageGameplayTags.h"
#include "Player/SFPlayerState.h"
#include "Messages/SFSkillInfoMessages.h"

void USFOverlayWidgetController::BroadcastInitialSets()
{
	// 초기 TargetPrimarySet 정보를 UI에 전달
	OnHealthChanged.Broadcast(TargetPrimarySet->GetHealth());
	OnMaxHealthChanged.Broadcast(TargetPrimarySet->GetMaxHealth());
	OnManaChanged.Broadcast(TargetPrimarySet->GetMana());
	OnMaxManaChanged.Broadcast(TargetPrimarySet->GetMaxMana());
	OnStaminaChanged.Broadcast(TargetPrimarySet->GetStamina());
	OnMaxStaminaChanged.Broadcast(TargetPrimarySet->GetMaxStamina());

	ASFPlayerState* SFPS = Cast<ASFPlayerState>(TargetPlayerState);
	if (SFPS)
	{
		OnPlayerInfoChanged.Broadcast(SFPS->GetPlayerSelection());
	}
}

void USFOverlayWidgetController::BindCallbacksToDependencies()
{
	BindPrimaryAttributeCallbacks();
	
	ASFPlayerState* SFPS = Cast<ASFPlayerState>(TargetPlayerState);
	if (SFPS)
	{
		SFPS->OnPlayerInfoChanged.AddDynamic(this, &ThisClass::HandlePlayerInfoChanged);
	}

	if (USFAbilitySystemComponent* SFASC = Cast<USFAbilitySystemComponent>(TargetAbilitySystemComponent))
	{
		SFASC->AbilityChangedDelegate.AddUObject(this, &ThisClass::HandleAbilityChanged);
	}

	// GE 제거 감지 
	if (TargetAbilitySystemComponent)
	{
		TargetAbilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().AddUObject(
			this, &ThisClass::HandleGameplayEffectRemoved);
	}

	// GE 스택 추가 감지
	if (UWorld* World = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetWorld() : nullptr)
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(World);
		ChainStateListenerHandle = MessageSubsystem.RegisterListener(
			SFGameplayTags::Message_Skill_ChainStateChanged,
			this,
			&ThisClass::HandleChainStateChangedMessage);
	}
	
	// TODO : 다른 Model들에 대한 Callbacks도 추가할 것
}

void USFOverlayWidgetController::BindPrimaryAttributeCallbacks()
{
	TWeakObjectPtr<USFOverlayWidgetController> WeakThis(this);
	if (TargetPrimarySet && TargetAbilitySystemComponent)
	{
		TargetAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(TargetPrimarySet->GetHealthAttribute()).AddLambda(
			[WeakThis](const FOnAttributeChangeData& Data)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OnHealthChanged.Broadcast(Data.NewValue);
			}
		});
		TargetAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(TargetPrimarySet->GetMaxHealthAttribute()).AddLambda(
			[WeakThis](const FOnAttributeChangeData& Data)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OnMaxHealthChanged.Broadcast(Data.NewValue);
			}
		});
		TargetAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(TargetPrimarySet->GetManaAttribute()).AddLambda(
			[WeakThis](const FOnAttributeChangeData& Data)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OnManaChanged.Broadcast(Data.NewValue);
			}
		});
		TargetAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(TargetPrimarySet->GetMaxManaAttribute()).AddLambda(
			[WeakThis](const FOnAttributeChangeData& Data)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OnMaxManaChanged.Broadcast(Data.NewValue);
			}
		});
		TargetAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(TargetPrimarySet->GetStaminaAttribute()).AddLambda(
			[WeakThis](const FOnAttributeChangeData& Data)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OnStaminaChanged.Broadcast(Data.NewValue);
			}
		});
		TargetAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(TargetPrimarySet->GetMaxStaminaAttribute()).AddLambda(
			[WeakThis](const FOnAttributeChangeData& Data)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OnMaxStaminaChanged.Broadcast(Data.NewValue);
			}
		});
	}
}

void USFOverlayWidgetController::HandlePlayerInfoChanged(const FSFPlayerSelectionInfo& NewPlayerSelection)
{
	OnPlayerInfoChanged.Broadcast(NewPlayerSelection);
}

void USFOverlayWidgetController::HandleAbilityChanged(FGameplayAbilitySpecHandle AbilitySpecHandle, bool bGiven)
{
	OnAbilityChanged.Broadcast(AbilitySpecHandle, bGiven);
}

void USFOverlayWidgetController::HandleChainStateChangedMessage(FGameplayTag Channel, const FSFChainStateChangedMessage& Message)
{
	OnChainStateChanged.Broadcast(Message.AbilitySpecHandle, Message.ChainIndex);
}

void USFOverlayWidgetController::HandleGameplayEffectRemoved(const FActiveGameplayEffect& RemovedEffect)
{
	BroadcastChainStateForAbility(RemovedEffect.Spec.Def, 0);
}

void USFOverlayWidgetController::BroadcastChainStateForAbility(const UGameplayEffect* EffectDef, int32 StackCount)
{
	if (!TargetAbilitySystemComponent || !EffectDef)
	{
		return;
	}

	for (const FGameplayAbilitySpec& Spec : TargetAbilitySystemComponent->GetActivatableAbilities())
	{
		const ISFChainedSkill* ChainedSkill = Cast<ISFChainedSkill>(Spec.Ability);
		if (!ChainedSkill)
		{
			continue;
		}

		TSubclassOf<UGameplayEffect> ComboStateClass = ChainedSkill->GetComboStateEffectClass();
        
		if (ComboStateClass && ComboStateClass == EffectDef->GetClass())
		{
			OnChainStateChanged.Broadcast(Spec.Handle, StackCount);
			break;
		}
	}
}



