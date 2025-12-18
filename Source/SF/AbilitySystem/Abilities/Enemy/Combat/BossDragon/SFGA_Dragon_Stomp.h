// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"
#include "SFGA_Dragon_Stomp.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Dragon_Stomp : public USFGA_Enemy_BaseAttack
{
	GENERATED_BODY()

public:
	USFGA_Dragon_Stomp();


	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;


protected:
	UFUNCTION()
	void OnMontageCompleted();
	
	UFUNCTION()
	void OnMontageInterrupted();
	
	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void EmitShockWave(FGameplayEventData Payload);
	void ApplyKnockBackToTarget(AActor* Target, const FVector& StompLocation);

protected:
	
	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Montage")
	TObjectPtr<UAnimMontage> StompMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Trace")
	float ShockwaveRadius = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Trace")
	bool bIsDebug = false;
	
	

	

	
	
		
	
};
