// Fill out your copyright notice in the Description page of Project Settings.


#include "SFCombatSet_Hero.h"

#include "Net/UnrealNetwork.h"

USFCombatSet_Hero::USFCombatSet_Hero()
{
}

void USFCombatSet_Hero::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Luck, COND_None, REPNOTIFY_Always);
}

bool USFCombatSet_Hero::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	return Super::PreGameplayEffectExecute(Data);
}

void USFCombatSet_Hero::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}

void USFCombatSet_Hero::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
}

void USFCombatSet_Hero::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
}

void USFCombatSet_Hero::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
}

void USFCombatSet_Hero::OnRep_Luck(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Luck, OldValue)
}
