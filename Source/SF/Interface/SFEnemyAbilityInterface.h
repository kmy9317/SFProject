// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayAbilitySpec.h"
#include "AI/Controller/Dragon/SFDragonCombatComponent.h"
#include "SFEnemyAbilityInterface.generated.h"


struct FEnemyAbilitySelectContext
{
	const FGameplayAbilitySpec* AbilitySpec = nullptr;
    
	AActor* Self = nullptr;
	AActor* Target = nullptr;
    
	
	float DistanceToTarget = 0.f;
	float AngleToTarget = 0.f;
    
	bool bMustFirst = false;
};

struct FBossEnemyAbilitySelectContext : public FEnemyAbilitySelectContext
{
	float PlayerHealthPercentage = 0.0f;
	EBossAttackZone Zone = EBossAttackZone::None;
	
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USFEnemyAbilityInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SF_API ISFEnemyAbilityInterface
{
	GENERATED_BODY()

	// This function does not need to be modified.
public:

	virtual float CalcAIScore(const FEnemyAbilitySelectContext& Context) const = 0;

	// 이건 특수 Ability 전용 
	virtual float CalcScoreModifier(const FEnemyAbilitySelectContext& Context) const =0;  
	
	
	
};
