// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayAbilitySpec.h"
#include "SFEnemyAbilityInterface.generated.h"


struct FEnemyAbilitySelectContext
{
	const FGameplayAbilitySpec* AbilitySpec = nullptr;
	
	AActor* Target = nullptr;
	AActor* Self = nullptr;
	
	bool bMustFirst = false;
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
	
	
	
};
