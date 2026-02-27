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

private:

	// 기본 데미지 계산
	float CalculateBaseDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FGameplayEffectSpec& Spec) const;

	// 크리티컬 적용
	float ApplyCritical(const FGameplayEffectCustomExecutionParameters& ExecutionParams,const FGameplayEffectSpec& Spec,float InDamage) const;

	// 방어력 적용
	float ApplyDefense(const FGameplayEffectCustomExecutionParameters& ExecutionParams,float InDamage) const;

	float ApplyIncomingDamageMultiplier(const FGameplayEffectCustomExecutionParameters& ExecutionParams, float InDamage) const;

	//최종 데미지 
	void OutputFinalDamage( float FinalDamage, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const;
};


