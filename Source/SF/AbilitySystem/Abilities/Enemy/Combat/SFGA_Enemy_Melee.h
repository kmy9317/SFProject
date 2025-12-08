// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"
#include "SFGA_Enemy_Melee.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Enemy_Melee : public USFGA_Enemy_BaseAttack
{
	GENERATED_BODY()
public:
	USFGA_Enemy_Melee(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
protected:
	UFUNCTION()
	void OnMontageCompleted();
	
	UFUNCTION()
	void OnMontageInterrupted();
	
	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnTraceHit(FGameplayEventData Payload);
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	void CleanupWeaponTraces();

	void ApplyParriedEffectToSelf();

protected:
	UPROPERTY(EditDefaultsOnly)
	uint8 Penetration; // 관통 숫자

	uint8 CurrentPenetration;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> ParriedGameplayEffectClass;

	// Ability Tasks
	UPROPERTY()
	class UAbilityTask_PlayMontageAndWait* MontageTask;

	UPROPERTY()
	class UAbilityTask_WaitGameplayEvent* EventTask;

};
