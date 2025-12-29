#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "SFGA_EnemyTurnInPlace.generated.h"

class UAbilityTask_PlayMontageAndWait;


UCLASS()
class SF_API USFGA_EnemyTurnInPlace : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_EnemyTurnInPlace();

protected:
	// ===== GAS Lifecycle =====

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	// ===== Montage Callbacks =====

	UFUNCTION()
	void OnTurnComplete();

	UFUNCTION()
	void OnTurnCancelled();

	UFUNCTION()
	void OnTurnInterrupted();

	
	UPROPERTY(EditDefaultsOnly, Category = "Turn In Place",
		meta = (Categories = "GameplayEvent.Turn"))
	TMap<FGameplayTag, UAnimMontage*> TurnMontageMap;


	UPROPERTY(EditDefaultsOnly, Category = "Turn In Place",
		meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float MontagePlayRate = 1.0f;

private:

	UPROPERTY()
	UAbilityTask_PlayMontageAndWait* CurrentMontageTask = nullptr;


	float ActualTurnYaw = 0.0f;

	
	float TargetYaw = 0.0f;

	
	FGameplayTag TriggerEventTag;



	bool ValidateTriggerEvent(const FGameplayEventData* TriggerEventData);
	bool StartTurnMontage(const FGameplayTag& EventTag);
	void CleanupMontageTask();
	void NotifyTurnFinished();
};
