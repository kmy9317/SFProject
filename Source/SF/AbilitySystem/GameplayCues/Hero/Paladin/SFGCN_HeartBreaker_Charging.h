// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Looping.h"
#include "SFGCN_HeartBreaker_Charging.generated.h"

class ASFEquipmentBase;
class ASFCharacterBase;
class UNiagaraComponent;

// BodyMesh 나이아가라 Phase별 파라미터
USTRUCT(BlueprintType)
struct FSFBodyChargingPhaseParams
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Phase")
	bool bAddDetail = false;

	UPROPERTY(EditDefaultsOnly, Category = "Phase")
	bool bSimple = false;

	UPROPERTY(EditDefaultsOnly, Category = "Phase")
	FLinearColor ColorTint = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "Phase")
	float Size = 0.5f;
};

// Weapon Aura 나이아가라 Phase별 파라미터
USTRUCT(BlueprintType)
struct FSFWeaponAuraPhaseParams
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, Category = "Phase")
	FLinearColor ColorTint = FLinearColor::White;
};

UCLASS()
class SF_API ASFGCN_HeartBreaker_Charging : public AGameplayCueNotify_Looping
{
	GENERATED_BODY()

public:
	ASFGCN_HeartBreaker_Charging();

protected:
	
	// Cue 활성 상태 유지 중
	virtual bool WhileActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;

	// ExecuteGameplayCue 호출 시 (차지 Phase 전환)
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;

	// ExecuteGameplayCue 호출 시 (차지 Phase 전환)
	virtual bool OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;

	UFUNCTION(BlueprintCallable, Category = "SF|Effects")
	void UpdateNiagaraPhaseParameters(int32 Phase);

	bool IsLocallyControlled(AActor* Target) const;

	void StartChargingSound();
	void StopChargingSound();
	void PlayPhaseTransitionSound(int32 Phase);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "SF|Effects")
	TObjectPtr<UNiagaraSystem> WeaponAuraSystem;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Effects")
	TObjectPtr<UNiagaraSystem> BodyMeshChargingSystem;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Attachment")
	FName WeaponAuraSocket = TEXT("AuraEffectSocket");

	UPROPERTY(EditDefaultsOnly, Category = "SF|Attachment")
	FName BodyChargingSocket = TEXT("ChargingEffectSocket");

	UPROPERTY(EditDefaultsOnly, Category = "SF|Parameters|Body")
	FName BodyParam_AddDetail = TEXT("AddDetail");

	UPROPERTY(EditDefaultsOnly, Category = "SF|Parameters|Body")
	FName BodyParam_Simple = TEXT("Simple");

	UPROPERTY(EditDefaultsOnly, Category = "SF|Parameters|Body")
	FName BodyParam_ColorTint = TEXT("ColorTint");

	UPROPERTY(EditDefaultsOnly, Category = "SF|Parameters|Body")
	FName BodyParam_Size = TEXT("_Size");

	UPROPERTY(EditDefaultsOnly, Category = "SF|Phase Settings", meta = (TitleProperty = "Phase {ArrayIndex}"))
	TArray<FSFBodyChargingPhaseParams> BodyPhaseParams;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Phase Settings", meta = (TitleProperty = "Phase {ArrayIndex}"))
	TArray<FSFWeaponAuraPhaseParams> WeaponAuraParams;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Sound")
	TObjectPtr<USoundBase> ChargingLoopSound;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Sound")
	TArray<TObjectPtr<USoundBase>> PhaseTransitionSounds;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Sound", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float ChargingLoopVolume = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Sound", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float ChargingLoopPitch = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Sound", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float PhaseTransitionVolume = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Sound", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float PhaseTransitionPitch = 1.0f;

private:

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> WeaponAuraComponent;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> BodyMeshChargingComponent;

	UPROPERTY()
	TObjectPtr<UAudioComponent> ChargingAudioComponent;

	UPROPERTY()
	TWeakObjectPtr<ASFCharacterBase> CachedTargetCharacter;

	UPROPERTY()
	TWeakObjectPtr<ASFEquipmentBase> CachedTargetWeapon;

	int32 CurrentChargePhase = 0;
	bool bIsLocalPlayer = false;
};
