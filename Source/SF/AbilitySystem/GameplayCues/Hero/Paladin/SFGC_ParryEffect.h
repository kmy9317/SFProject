#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Burst.h"
#include "SFGC_ParryEffect.generated.h"

class UParticleSystem;
class USoundBase;

UCLASS()
class SF_API USFGC_ParryEffect : public UGameplayCueNotify_Burst
{
	GENERATED_BODY()

protected:

	//====================== FX ======================
	UPROPERTY(EditDefaultsOnly, Category="SF|Parry|FX")
	UParticleSystem* ParryFX; // Cascade
	//================================================

	//==================== Sound =====================
	UPROPERTY(EditDefaultsOnly, Category="SF|Parry|Sound")
	USoundBase* ParrySound;
	//================================================

	//================= Location Adjust =================
	// Parameters.Location 기준 월드 오프셋
	UPROPERTY(EditDefaultsOnly, Category="SF|Parry|Location")
	FVector LocationOffset = FVector::ZeroVector;

	// 캐릭터 전방 기준 오프셋
	UPROPERTY(EditDefaultsOnly, Category="SF|Parry|Location")
	bool bUseForwardOffset = false;

	UPROPERTY(EditDefaultsOnly, Category="SF|Parry|Location", meta=(EditCondition="bUseForwardOffset"))
	float ForwardOffset = 0.f;
	//==================================================

	//==================== Scale ======================
	// 파티클 전체 스케일
	UPROPERTY(EditDefaultsOnly, Category="SF|Parry|Scale", meta=(ClampMin="0.1"))
	float ParticleScale = 1.0f;
	//==================================================

public:
	virtual bool OnExecute_Implementation(
		AActor* Target,
		const FGameplayCueParameters& Parameters
	) const override;
};
