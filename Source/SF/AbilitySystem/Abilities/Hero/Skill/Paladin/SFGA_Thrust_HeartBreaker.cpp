// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGA_Thrust_HeartBreaker.h"

void USFGA_Thrust_HeartBreaker::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void USFGA_Thrust_HeartBreaker::OnKeyReleased(float TimeHeld)
{
	
}
