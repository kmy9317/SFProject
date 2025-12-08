// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_EnemyParriedStagger.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_EnemyParriedStagger : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_EnemyParriedStagger();

	UFUNCTION()
	void OnMontageCompleted();
	UFUNCTION()
	void OnMontageInterrupted();
	UFUNCTION()
	void OnMontageCancelled();
	
	virtual void ActivateAbility( const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	virtual void EndAbility( const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer CancelTags;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> ParriedStaggerMontage;

	class UAbilityTask_PlayMontageAndWait* MontageTask;
};
