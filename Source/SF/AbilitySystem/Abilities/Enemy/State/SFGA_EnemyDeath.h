// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_EnemyDeath.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_EnemyDeath : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_EnemyDeath();

	
	virtual void ActivateAbility( const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	
	virtual void DeathEventAfterDelay();
	
	void DestroyEnemy();
	
	FTimerHandle EventTimerHandle;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	float EventTime = 5.f;
};
