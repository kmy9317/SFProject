// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"
#include "SFGA_Dragon_Bite.generated.h"

/**
 * Dragon Bite Attack Ability
 * Monster Hunter style Grab & Rescue mechanic
 */
UCLASS()
class SF_API USFGA_Dragon_Bite : public USFGA_Enemy_BaseAttack
{
	GENERATED_BODY()

public:
	USFGA_Dragon_Bite();

	virtual void ActivateAbility(	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
protected:
	UFUNCTION()
	void OnBiteMontageCompleted();

	UFUNCTION()
	void OnGrabMontageCompleted();
	
	UFUNCTION()
	void OnMontageInterrupted();
	
	UFUNCTION()
	void OnMontageCancelled();
	

	UFUNCTION()
	void OnBiteHit(FGameplayEventData Payload);

	void ApplyGrabEffect(AActor* Target);
	
	void AttachTargetToJaw(AActor* Target);

	void DetachTarget(AActor* Target);

	void PlayGrabMontage();

	UFUNCTION()
	void OnDamageRecieved(UAbilitySystemComponent* Source, const FGameplayEffectSpec& SpecApplied,FActiveGameplayEffectHandle ActiveHandle);

	void ApplyExecutionDamage(AActor* Target);

	void ApplyStaggerToSelf();


protected:
	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Montage")
	TObjectPtr<UAnimMontage> BiteMontage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Montage")
	TObjectPtr<UAnimMontage> GrabMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Effect")
	TSubclassOf<UGameplayEffect> GrabGameplayEffectClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Effect")
	TSubclassOf<UGameplayEffect> StaggerGameplayEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Grab")
	FName JawSocketName = "";

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Bite")
	int32 BiteCount = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Grab")
	float GrabDuration = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Rescue")
	int32 RescueCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Rescue")
	float DamageCountCoolDown = 0.5f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Grab")
	float ExecutionDamage = 9999.f;

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Grab")
	float StaggerDamageOnRescue = 50.f;

private:

	UPROPERTY()
	TWeakObjectPtr<AActor> GrabbedTarget;
	
	int32 CurrentBiteCount = 0;

	float LastDamageTime = -999.f;

	int32 CurrentHitCount = 0;
	
	FTimerHandle GrabDurationTimerHandle;
	
	FDelegateHandle OnDamageRecivedHandle;
	
	

};

