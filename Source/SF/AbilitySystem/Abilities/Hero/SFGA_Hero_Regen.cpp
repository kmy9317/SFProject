// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGA_Hero_Regen.h"

#include "AbilitySystemComponent.h"

USFGA_Hero_Regen::USFGA_Hero_Regen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = ESFAbilityActivationPolicy::OnSpawn;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void USFGA_Hero_Regen::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 스태미나 리젠 적용
	if (StaminaRegenEffectClass)
	{
		StaminaRegenHandle = ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, StaminaRegenEffectClass.GetDefaultObject(), GetAbilityLevel());
	}

	// 마나 리젠 적용
	if (ManaRegenEffectClass)
	{
		ManaRegenHandle = ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, ManaRegenEffectClass.GetDefaultObject(), GetAbilityLevel());
	}

}

void USFGA_Hero_Regen::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		if (StaminaRegenHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(StaminaRegenHandle);
			StaminaRegenHandle.Invalidate();
		}

		if (ManaRegenHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(ManaRegenHandle);
			ManaRegenHandle.Invalidate();
		}
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
