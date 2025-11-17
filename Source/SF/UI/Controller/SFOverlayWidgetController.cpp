// Fill out your copyright notice in the Description page of Project Settings.


#include "SFOverlayWidgetController.h"

#include "AbilitySystem/Attributes/Hero/SFPrimarySet_Hero.h"
#include "Player/SFPlayerState.h"

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


