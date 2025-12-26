// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_Hero_StageClearHealing.generated.h"

UCLASS()
class SF_API USFGA_Hero_StageClearHealing : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Hero_StageClearHealing();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="StageClear")
	TSubclassOf<UGameplayEffect> StageClearHealEffect;
};
