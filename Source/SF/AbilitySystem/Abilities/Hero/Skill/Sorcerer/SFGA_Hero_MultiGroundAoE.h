#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Hero_GroundAoE.h"
#include "SFGA_Hero_MultiGroundAoE.generated.h"

class ASFMultiGroundActor;

/**
 * 다중 지면 타격 스킬 (Lightning Storm)
 * - 정해진 범위(AOERadius) 내에서 랜덤한 위치에 번개를 여러 번 떨어뜨림
 */
UCLASS()
class SF_API USFGA_Hero_MultiGroundAoE : public USFGA_Hero_GroundAoE
{
	GENERATED_BODY()

public:
	USFGA_Hero_MultiGroundAoE(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// === 설정 변수 ===

	// 떨어질 번개 액터 클래스 (SFMultiGroundActor 상속)
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Lightning")
	TSubclassOf<ASFMultiGroundActor> LightningActorClass;

	// 총 떨어질 번개 횟수
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Lightning")
	int32 LightningCount = 10;

	// 번개 소환 주기 (초)
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Lightning")
	float LightningSpawnInterval = 0.2f;

	// 개별 번개의 반경 (두께)
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Lightning")
	float LightningBoltRadius = 50.f;
	
	// 번개의 높이 (충돌체 높이)
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Lightning")
	float LightningBoltHeight = 500.f;

	// 번개 크기 랜덤 스케일 (최소)
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Lightning")
	float MinScaleMultiplier = 0.8f;

	// 번개 크기 랜덤 스케일 (최대)
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Lightning")
	float MaxScaleMultiplier = 1.5f;

protected:
	// 부모의 단일 소환 로직을 대체하여 다중 소환 타이머 시작
	virtual void OnSpawnEventReceived(FGameplayEventData Payload) override;

	// 타이머에 의해 주기적으로 호출될 함수
	UFUNCTION()
	void SpawnSingleLightning();

private:
	FTimerHandle LightningTimerHandle;
	int32 CurrentLightningCount = 0;
};