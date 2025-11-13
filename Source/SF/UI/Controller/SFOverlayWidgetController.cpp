// Fill out your copyright notice in the Description page of Project Settings.


#include "SFOverlayWidgetController.h"

#include "AbilitySystem/Attributes/Hero/SFPrimarySet_Hero.h"

void USFOverlayWidgetController::BroadcastInitialValues()
{
	// 초기 PrimarySet 정보를 UI에 전달
	OnHealthChanged.Broadcast(PrimarySet->GetHealth());
	OnMaxHealthChanged.Broadcast(PrimarySet->GetMaxHealth());
	OnManaChanged.Broadcast(PrimarySet->GetMana());
	OnMaxManaChanged.Broadcast(PrimarySet->GetMaxMana());
	OnStaminaChanged.Broadcast(PrimarySet->GetStamina());
	OnMaxStaminaChanged.Broadcast(PrimarySet->GetMaxStamina());
}

void USFOverlayWidgetController::BindCallbacksToDependencies()
{
	BindPrimaryAttributeCallbacks();
	// TODO : 다른 Model들에 대한 Callbacks도 추가할 것
}

void USFOverlayWidgetController::BindPrimaryAttributeCallbacks()
{
	TWeakObjectPtr<USFOverlayWidgetController> WeakThis(this);
	if (PrimarySet && AbilitySystemComponent)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PrimarySet->GetHealthAttribute()).AddLambda(
			[WeakThis](const FOnAttributeChangeData& Data)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OnHealthChanged.Broadcast(Data.NewValue);
			}
		});
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PrimarySet->GetMaxHealthAttribute()).AddLambda(
			[WeakThis](const FOnAttributeChangeData& Data)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OnMaxHealthChanged.Broadcast(Data.NewValue);
			}
		});
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PrimarySet->GetManaAttribute()).AddLambda(
			[WeakThis](const FOnAttributeChangeData& Data)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OnManaChanged.Broadcast(Data.NewValue);
			}
		});
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PrimarySet->GetMaxManaAttribute()).AddLambda(
			[WeakThis](const FOnAttributeChangeData& Data)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OnMaxManaChanged.Broadcast(Data.NewValue);
			}
		});
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PrimarySet->GetStaminaAttribute()).AddLambda(
			[WeakThis](const FOnAttributeChangeData& Data)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OnStaminaChanged.Broadcast(Data.NewValue);
			}
		});
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PrimarySet->GetMaxStaminaAttribute()).AddLambda(
			[WeakThis](const FOnAttributeChangeData& Data)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OnMaxStaminaChanged.Broadcast(Data.NewValue);
			}
		});
	}
}


