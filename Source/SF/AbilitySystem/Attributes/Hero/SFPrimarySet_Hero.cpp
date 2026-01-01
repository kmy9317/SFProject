#include "SFPrimarySet_Hero.h"

#include "GameplayEffectExtension.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Libraries/SFAbilitySystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/Components/SFPlayerCombatStateComponent.h"

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
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, CooldownRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, ManaRegen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, StaminaRegen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, ManaReduction, COND_None, REPNOTIFY_Always);
}

bool USFPrimarySet_Hero::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent();
		if (SFASC && SFASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Downed))
		{
			return false;
		}
	}

	return true;
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

void USFPrimarySet_Hero::HandleZeroHealth(USFAbilitySystemComponent* SFASC, const FGameplayEffectModCallbackData& Data)
{
	if (CanEnterDownedState())
	{
		USFAbilitySystemLibrary::SendDownedEventFromSpec(SFASC, Data.EffectSpec);
	}
	else
	{
		Super::HandleZeroHealth(SFASC, Data);
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
	else if (Attribute == GetCooldownRateAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 0.8f);
	}
	else if (Attribute == GetManaRegenAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 10.0f);
	}
	else if (Attribute == GetStaminaRegenAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 10.0f);
	}
	else if (Attribute == GetManaReductionAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 0.9f);
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

void USFPrimarySet_Hero::OnRep_CooldownRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, CooldownRate, OldValue);
}

void USFPrimarySet_Hero::OnRep_ManaRegen(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, ManaRegen, OldValue);
}

void USFPrimarySet_Hero::OnRep_StaminaRegen(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, StaminaRegen, OldValue);
}

void USFPrimarySet_Hero::OnRep_ManaReduction(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, ManaReduction, OldValue);
}

bool USFPrimarySet_Hero::CanEnterDownedState() const
{
	USFPlayerCombatStateComponent* CombatState = USFPlayerCombatStateComponent::FindPlayerCombatStateComponent(GetOwningActor());
	if (!CombatState)
	{
		return false;
	}

	return CombatState->GetRemainingDownCount() > 0;
}
