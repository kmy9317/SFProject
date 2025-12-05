// Fill out your copyright notice in the Description page of Project Settings.


#include "SFPrimarySet.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

#include "GameplayEffectExtension.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterGameplayTags.h"

USFPrimarySet::USFPrimarySet()
{
}

void USFPrimarySet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MoveSpeed, COND_None, REPNOTIFY_Always);
}

bool USFPrimarySet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	return Super::PreGameplayEffectExecute(Data);
}

void USFPrimarySet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 변경된 Attribute가 'Damage'인지 확인
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		// 1. 데미지 값(DamageDone)을 로컬 변수에 저장
		const float DamageDone = GetDamage();
		// 2. 임시 Attribute인 Damage를 다시 0으로 초기화
		SetDamage(0.0f);

		// 3.체력이 0보다 크고 유효한 데미지가 들어왔는지 확인
		if (DamageDone > 0.0f && GetHealth() > 0.0f)
		{
			// 4. 'Damage'를 'Health'에 적용
			SetHealth(GetHealth() - DamageDone);
		}
	}

	// 변경된 Attribute가 'Health'인지 확인
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// 5. 체력이 0 이하가 되었는지 확인 (사망)
		if (GetHealth() <= 0.0f)
		{
			if (USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent())
			{
				// 6. 사망 태그가 없는 경우에만 부여 (중복방지)
				if (!SFASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
				{
					SFASC->AddLooseGameplayTag(SFGameplayTags::Character_State_Dead);

					// TODO: 후에 GA_Death와 같은 사망 전용 어빌리티 활성화
					 FGameplayEventData Payload;
					 SFASC->HandleGameplayEvent(SFGameplayTags::GameplayEvent_Death, &Payload);
				}
			}
		}
	}
}

void USFPrimarySet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void USFPrimarySet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void USFPrimarySet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxHealthAttribute())
	{
		if (GetHealth() > NewValue)
		{
			USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent();
			check(SFASC);

			SFASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Override, NewValue);
		}
	}
}

void USFPrimarySet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}

void USFPrimarySet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Health, OldValue);
}

void USFPrimarySet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxHealth, OldValue);
}

void USFPrimarySet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MoveSpeed, OldValue);
}
