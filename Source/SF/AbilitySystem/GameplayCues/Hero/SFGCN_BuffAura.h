#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "SFGCN_BuffAura.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UParticleSystem; // 캐스케이드용 전방 선언
class UParticleSystemComponent; // 캐스케이드용 전방 선언

UCLASS()
class SF_API ASFGCN_BuffAura : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	ASFGCN_BuffAura();

protected:
	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	// SourceObject가 없을 때 사용할 기본 FX (나이아가라)
	UPROPERTY(EditDefaultsOnly, Category="SF|VFX")
	TObjectPtr<UNiagaraSystem> DefaultNiagaraSystem;

	// [추가됨] SourceObject가 없을 때 사용할 기본 FX (캐스케이드)
	UPROPERTY(EditDefaultsOnly, Category="SF|VFX")
	TObjectPtr<UParticleSystem> DefaultCascadeSystem;

	UPROPERTY(EditDefaultsOnly, Category="SF|SFX")
	TObjectPtr<USoundBase> DefaultLoopSound;

	UPROPERTY(EditDefaultsOnly, Category="SF|VFX")
	FName AttachSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category="SF|VFX")
	FVector LocationOffset = FVector(0.f, 0.f, 0.f);

	UPROPERTY(EditDefaultsOnly, Category="SF|VFX")
	FRotator RotationOffset = FRotator::ZeroRotator;
	
	UPROPERTY(EditDefaultsOnly, Category="SF|VFX")
	FVector FXScale = FVector(1.f);

private:
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> SpawnedNiagaraComponent;

	// [추가됨] 생성된 캐스케이드 컴포넌트 포인터
	UPROPERTY()
	TObjectPtr<UParticleSystemComponent> SpawnedCascadeComponent;

	UPROPERTY()
	TObjectPtr<UAudioComponent> SpawnedAudioComponent;
};