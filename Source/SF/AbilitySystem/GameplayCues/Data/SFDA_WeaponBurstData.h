#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SFDA_WeaponBurstData.generated.h"

class UNiagaraSystem;
class UParticleSystem;
class USoundBase;

/**
 * 무기 이펙트(발사, 타격 등) 설정을 정의하는 데이터 에셋
 * GameplayCue 실행 시 SourceObject로 전달하여 사용합니다.
 */
UCLASS()
class SF_API USFDA_WeaponBurstData : public UDataAsset
{
	GENERATED_BODY()

public:
	// ------------------------------------------------------
	// VFX (Visual Effects)
	// ------------------------------------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> NiagaraVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UParticleSystem> CascadeVFX;

	// 무기의 어떤 소켓에서 발생시킬지 (예: Muzzle, Tip)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	FName SocketName = FName("Muzzle");

	// 이펙트가 소켓에 붙어서 따라다닐지 여부 (MuzzleFlash는 True, 타격점은 False 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	bool bAttachToSocket = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	FVector Scale = FVector(1.0f);

	// ------------------------------------------------------
	// SFX (Sound Effects)
	// ------------------------------------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	float PitchMultiplier = 1.0f;
};