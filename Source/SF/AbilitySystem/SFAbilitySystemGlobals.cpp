// Fill out your copyright notice in the Description page of Project Settings.


#include "SFAbilitySystemGlobals.h"
#include "GameplayEffect/SFGameplayEffectContext.h"


FGameplayEffectContext* USFAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FSFGameplayEffectContext();
}
