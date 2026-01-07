#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "SFGC_FXSpawnFloor.generated.h"

class UNiagaraComponent;
class UParticleSystemComponent;
class UAudioComponent;

/**
 * 캐릭터 바닥에 장판형 스킬 이펙트(Niagara/Cascade/Sound)를 소환하는 GameplayCue
 */
UCLASS()
class SF_API ASFGC_FXSpawnFloor : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	ASFGC_FXSpawnFloor();

protected:
	// GameplayCue가 활성화될 때 (GE 적용 시)
	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

	// GameplayCue가 제거될 때 (GE 만료/제거 시)
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

public:
	// --- 컴포넌트 ---
	
	// 나이아가라 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> NiagaraComp;

	// 캐스케이드 파티클 컴포넌트 (레거시 지원용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UParticleSystemComponent> CascadeComp;

	// 사운드 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAudioComponent> AudioComp;

	// --- 설정 변수 (블루프린트에서 수정 가능) ---

	// true: 캐릭터가 이동하면 이펙트도 따라감 / false: 처음 소환된 위치에 고정
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FX Settings")
	bool bFollowCharacter = false;

	// 바닥 위치 보정값 (예: 0이면 발바닥, 조금 띄우려면 양수)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FX Settings")
	FVector FloorOffset = FVector(0.f, 0.f, 0.f);

	// 이펙트 크기 조절
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FX Settings")
	FVector EffectScale = FVector(1.f, 1.f, 1.f);

	// 이펙트 추가 회전값
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FX Settings")
	FRotator AdditionalRotation = FRotator::ZeroRotator;
};