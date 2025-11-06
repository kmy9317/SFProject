#include "SFPrimarySet_Hero.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

USFPrimarySet_Hero::USFPrimarySet_Hero()
{
	
}

void USFPrimarySet_Hero::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxStamina, COND_None, REPNOTIFY_Always);
}

bool USFPrimarySet_Hero::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	return Super::PreGameplayEffectExecute(Data);
}

void USFPrimarySet_Hero::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}

void USFPrimarySet_Hero::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
}

void USFPrimarySet_Hero::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
}

void USFPrimarySet_Hero::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxManaAttribute())
	{
		if (GetMana() > NewValue)
		{
			USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent();
			check(SFASC);

			SFASC->ApplyModToAttribute(GetManaAttribute(), EGameplayModOp::Override, NewValue);
		}
	}
	else if (Attribute == GetMaxStaminaAttribute())
	{
		if (GetStamina() > NewValue)
		{
			USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent();
			check(SFASC);

			SFASC->ApplyModToAttribute(GetStaminaAttribute(), EGameplayModOp::Override, NewValue);
		}
	}
}

void USFPrimarySet_Hero::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::ClampAttribute(Attribute, NewValue);
	
	if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
	}
	else if (Attribute == GetMaxManaAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetMaxStaminaAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}

void USFPrimarySet_Hero::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Mana, OldValue);
}

void USFPrimarySet_Hero::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxMana, OldValue);
}

void USFPrimarySet_Hero::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Stamina, OldValue);
}

void USFPrimarySet_Hero::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxStamina, OldValue);
}
