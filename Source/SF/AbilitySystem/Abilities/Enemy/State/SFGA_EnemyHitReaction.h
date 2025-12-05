// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_EnemyHitReaction.generated.h"

class UAbilityTask_PlayMontageAndWait; 
/**
 * 
 */
UCLASS()
class SF_API USFGA_EnemyHitReaction : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_EnemyHitReaction();

protected:
	/** 피격 반응 시 재생할 애니메이션 몽타주 */
	UPROPERTY(EditDefaultsOnly, Category = "SFGA|HitReaction")
	UAnimMontage* FrontHitMontage;

	UPROPERTY(EditDefaultsOnly, Category = "SFGA|HitReaction")
	UAnimMontage* BackHitMontage;
	
	UAbilityTask_PlayMontageAndWait* MontageTask;

	/** Gameplay Effect가 적용된 핸들. EndAbility에서 제거하는 데 사용 */
	FActiveGameplayEffectHandle ActiveHitEffectHandle;
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		bool bReplicateEndAbility, bool bWasCancelled) override;



	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnMontageInterrupted();
	
	FVector ExtractHitLocationFromEvent(const FGameplayEventData* EventData) const;

	FVector ExtractHitDirectionFromEvent(const FGameplayEventData* EventData) const;
};
