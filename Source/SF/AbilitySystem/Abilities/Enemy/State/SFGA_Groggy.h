// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_Groggy.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Groggy : public USFGameplayAbility
{
	GENERATED_BODY()
public:
	USFGA_Groggy();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Groggy")
	float GroggyDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category= "Effect")
	TSubclassOf<UGameplayEffect> ResetEffect;
private:
	UFUNCTION()
	void OnGroggyTimeFinished();
};
