// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "SFDamageEffectExecCalculation.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFDamageEffectExecCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	USFDamageEffectExecCalculation();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
