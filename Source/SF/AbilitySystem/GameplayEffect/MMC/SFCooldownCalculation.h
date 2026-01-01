#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "SFCooldownCalculation.generated.h"

/**
 * 쿨타임 계산 클래스
 * 최종 쿨타임 = SetByCaller(기본 쿨타임) × CooldownRate
 */
UCLASS()
class SF_API USFCooldownCalculation : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
public:
	USFCooldownCalculation(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
	
};
