#include "SFCombatSet_Enemy.h"

#include "Net/UnrealNetwork.h"

USFCombatSet_Enemy::USFCombatSet_Enemy()
	: SightRadius(1500.0f)
	, LoseSightRadius(2000.0f)
	, MeleeRange(200.0f)
	, GuardRange(800.0f)
{
}

void USFCombatSet_Enemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, SightRadius, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, LoseSightRadius, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MeleeRange, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, GuardRange, COND_None, REPNOTIFY_Always);
}

void USFCombatSet_Enemy::OnRep_SightRadius(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, SightRadius, OldValue);
}

void USFCombatSet_Enemy::OnRep_LoseSightRadius(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, LoseSightRadius, OldValue);
}

void USFCombatSet_Enemy::OnRep_MeleeRange(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MeleeRange, OldValue);
}

void USFCombatSet_Enemy::OnRep_GuardRange(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, GuardRange, OldValue);
}

bool USFCombatSet_Enemy::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	return Super::PreGameplayEffectExecute(Data);
}

void USFCombatSet_Enemy::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}

void USFCombatSet_Enemy::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
}

void USFCombatSet_Enemy::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
}

void USFCombatSet_Enemy::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
}
