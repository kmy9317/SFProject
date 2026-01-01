// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_Hero_Base.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Hero_Base : public USFGameplayAbility
{
	GENERATED_BODY()
public:
	USFGA_Hero_Base(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "SF|Cooldown", meta = (DisplayName = "기본 쿨타임(초)"))
	FScalableFloat BaseCooldownDuration = 10.f;
};
