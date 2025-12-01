// Fill out your copyright notice in the Description page of Project Settings.

#include "SFEnemyAbilityInitializer.h"

#include "GameplayAbilitySpec.h"
#include "GameplayTagsManager.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AI/SFAIGameplayTags.h"

void USFEnemyAbilityInitializer::ApplyAbilityData(FGameplayAbilitySpec& Spec, const FAbilityBaseData& Data)
{
    static TMap<EAbilityType, TFunction<void(FGameplayAbilitySpec&, const FAbilityBaseData&)>> InitializerMap;
    
    if (InitializerMap.Num() == 0)
    {
        InitializeMap(InitializerMap);
    }

    if (InitializerMap.Contains(Data.AbilityType))
    {
        InitializerMap[Data.AbilityType](Spec, Data);
    }
}

void USFEnemyAbilityInitializer::InitializeMap(TMap<EAbilityType, TFunction<void(FGameplayAbilitySpec&, const FAbilityBaseData&)>>& Map)
{
    Map.Add(EAbilityType::Attack, [](FGameplayAbilitySpec& Spec, const FAbilityBaseData& Data)
    {
        InitializeAttackAbility(Spec, Data);
    });
    
}

void USFEnemyAbilityInitializer::InitializeAttackAbility(FGameplayAbilitySpec& Spec, const FAbilityBaseData& Data)
{
    const FEnemyAttackAbilityData& AttackData = static_cast<const FEnemyAttackAbilityData&>(Data);

    Spec.SetByCallerTagMagnitudes.Add(SFGameplayTags::Data_EnemyAbility_BaseDamage, AttackData.BaseDamage);
    Spec.SetByCallerTagMagnitudes.Add(SFGameplayTags::Data_EnemyAbility_AttackRange, AttackData.AttackRange);
    Spec.SetByCallerTagMagnitudes.Add(SFGameplayTags::Data_EnemyAbility_AttackAngle, AttackData.AttackAngle);
    Spec.SetByCallerTagMagnitudes.Add(SFGameplayTags::Data_EnemyAbility_MinAttackRange, AttackData.MinAttackRange);
    Spec.SetByCallerTagMagnitudes.Add(SFGameplayTags::Data_EnemyAbility_Cooldown, AttackData.Cooldown);
}

