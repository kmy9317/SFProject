#include "SFPrimarySet_Enemy.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

USFPrimarySet_Enemy::USFPrimarySet_Enemy()
{
}

void USFPrimarySet_Enemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Stagger, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxStagger, COND_None, REPNOTIFY_Always);
}

bool USFPrimarySet_Enemy::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	return Super::PreGameplayEffectExecute(Data);
}

void USFPrimarySet_Enemy::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}

void USFPrimarySet_Enemy::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
}

void USFPrimarySet_Enemy::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
}

void USFPrimarySet_Enemy::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxStaggerAttribute())
	{
		if (GetStagger() > NewValue)
		{
			USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent();
			check(SFASC);

			SFASC->ApplyModToAttribute(GetStaggerAttribute(), EGameplayModOp::Override, NewValue);
		}
	}
}

void USFPrimarySet_Enemy::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::ClampAttribute(Attribute, NewValue);

	if (Attribute == GetStaggerAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStagger());
	}
	else if (Attribute == GetMaxStaggerAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}

void USFPrimarySet_Enemy::OnRep_Stagger(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Stagger, OldValue);
}

void USFPrimarySet_Enemy::OnRep_MaxStagger(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxStagger, OldValue);
}
