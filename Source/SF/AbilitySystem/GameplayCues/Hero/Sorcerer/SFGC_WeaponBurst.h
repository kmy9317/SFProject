#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Burst.h"
#include "SFGC_WeaponBurst.generated.h"

class USFDA_WeaponBurstData;

/**
 * 단일 태그로 다양한 무기 이펙트를 처리하는 큐.
 * Context의 SourceObject로 SFDA_WeaponBurstData를 전달받습니다.
 */
UCLASS()
class SF_API USFGC_WeaponBurst : public UGameplayCueNotify_Burst
{
	GENERATED_BODY()

protected:
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;

	// ------------------------------------------------------------
	// Fallback Defaults (DataAsset이 전달되지 않았을 때 사용할 기본값)
	// ------------------------------------------------------------
	UPROPERTY(EditDefaultsOnly, Category = "SF|Default")
	TObjectPtr<UNiagaraSystem> DefaultNiagaraVFX;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Default")
	TObjectPtr<USoundBase> DefaultSound;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Default")
	FName DefaultSocketName = FName("Muzzle");
};