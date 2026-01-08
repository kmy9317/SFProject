#include "SFPrimarySet_Enemy.h"

#include "GameplayEffectExtension.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "System/SFGameInstance.h"
#include "Character/Enemy/SFEnemy.h"
#include "Libraries/SFAbilitySystemLibrary.h"

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
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent();
		if (SFASC && SFASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Groggy))
		{
			Data.EvaluatedData.Magnitude *= 1.5f;
		}
	}
    
	return Super::PreGameplayEffectExecute(Data);
}

void USFPrimarySet_Enemy::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float DamageDone = GetDamage();
		
		// 유효한 데미지가 들어왔는지 확인
		if (DamageDone > 0.0f)
		{
			AActor* Instigator = Data.EffectSpec.GetContext().GetInstigator();
			
			OnTakeDamageDelegate.Broadcast(DamageDone, Instigator);
		}
	}
	Super::PostGameplayEffectExecute(Data);

	if (Data.EffectSpec.SetByCallerTagMagnitudes.Contains(SFGameplayTags::Data_Stagger_BaseStagger))
	{
		const float AddedStagger =Data.EffectSpec.GetSetByCallerMagnitude(SFGameplayTags::Data_Stagger_BaseStagger, false);

		if (AddedStagger > 0.f)
		{
			const float NewStagger =FMath::Clamp(GetStagger() + AddedStagger, 0.f, GetMaxStagger());

			SetStagger(NewStagger);
			
			if (NewStagger >= GetMaxStagger())
			{
				USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent();
				if (SFASC)
				{
					USFAbilitySystemLibrary::SendStaggerEventFromSpec(SFASC, Data.EffectSpec);
				}
			}
		}
	}

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
