// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"
#include "Interface/ISFDragonPressureInterface.h"
#include "SFGA_Dragon_Bite.generated.h"

/**
 * Dragon Bite Attack Ability
 * Monster Hunter style Grab & Rescue mechanic
 * Implements ISFDragonPressureInterface to apply Forward Pressure
 */
UCLASS()
class SF_API USFGA_Dragon_Bite : public USFGA_Enemy_BaseAttack, public ISFDragonPressureInterface
{
	GENERATED_BODY()

public:
	USFGA_Dragon_Bite();

	// ISFDragonPressureInterface 구현
	virtual EDragonPressureType GetPressureType() const override { return EDragonPressureType::Forward; }
	virtual float GetPressureDuration() const override { return PressureDuration; }
	virtual TSubclassOf<UGameplayEffect> GetPressureEffectClass() const override { return PressureEffectClass; }

	// CalcScoreModifier 구현 (Pressure 기반 AI 점수 조정)
	virtual float CalcScoreModifier(const FEnemyAbilitySelectContext& Context) const override;

	virtual void ActivateAbility(	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
protected:
	// === Section Control Functions ===
	void StartBiteAttack();
	void PlayBiteLoop();
	void PlayGrabMontage();

	// === Montage Callbacks ===
	UFUNCTION()
	void OnBiteMontageCompleted();

	UFUNCTION()
	void OnGrabMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	// === Hit & Grab Logic ===
	UFUNCTION()
	void OnBiteHit(FGameplayEventData Payload);

	void ApplyGrabEffect(AActor* Target);

	void AttachTargetToJaw(AActor* Target);

	void DetachTarget(AActor* Target);

	// === Rescue Logic ===
	UFUNCTION()
	void OnDamageRecieved(UAbilitySystemComponent* Source, const FGameplayEffectSpec& SpecApplied,FActiveGameplayEffectHandle ActiveHandle);

	void ApplyExecutionDamage(AActor* Target);

	void ApplyStaggerToSelf();


protected:
	// === Montage ===
	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Montage")
	TObjectPtr<UAnimMontage> BiteMontage;

	// === GameplayEffect ===
	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Effect")
	TSubclassOf<UGameplayEffect> GrabGameplayEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Effect")
	TSubclassOf<UGameplayEffect> StaggerGameplayEffectClass;

	// === Bite Settings ===
	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Bite")
	int32 BiteCount = 2;

	// === Grab Settings ===
	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Grab")
	FName JawSocketName = "";

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Grab")
	float GrabDuration = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Grab")
	float ExecutionDamage = 9999.f;

	// === Rescue Settings ===
	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Rescue")
	int32 RescueCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Rescue")
	float DamageCountCoolDown = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Rescue")
	float StaggerDamageOnRescue = 50.f;

	// === Pressure Settings ===
	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Pressure")
	float PressureDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Pressure")
	TSubclassOf<UGameplayEffect> PressureEffectClass;

private:

	UPROPERTY()
	TWeakObjectPtr<AActor> GrabbedTarget;

	int32 CurrentBiteCount = 0;

	float LastDamageTime = -999.f;

	int32 CurrentHitCount = 0;

	FTimerHandle GrabDurationTimerHandle;

	FDelegateHandle OnDamageRecivedHandle;

	// Grab Effect Handle (태그 제거를 위해 저장)
	FActiveGameplayEffectHandle ActiveGrabEffectHandle;
	
	

};

