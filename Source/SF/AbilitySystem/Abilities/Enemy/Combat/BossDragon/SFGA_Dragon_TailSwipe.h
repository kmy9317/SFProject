// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"
#include "SFGA_Dragon_TailSwipe.generated.h"

/**
 * Dragon TailSwipe Attack Ability
 * Uses AnimNotifyState_SweepTrace to detect hits along the tail swing
 */
UCLASS()
class SF_API USFGA_Dragon_TailSwipe : public USFGA_Enemy_BaseAttack
{
	GENERATED_BODY()

public:
	USFGA_Dragon_TailSwipe();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
protected:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnTailHit(FGameplayEventData Payload);

	void ApplyKnockBackToTarget(AActor* Target, const FVector& HitLocation);

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Montage")
	TObjectPtr<UAnimMontage> TailSwipeMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Debug")
	bool bIsDebug = false;

private:
	// Task 참조 저장 (수동 종료를 위해)
	UPROPERTY()
	TObjectPtr<class UAbilityTask_WaitGameplayEvent> WaitEventTask;
};
