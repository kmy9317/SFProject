#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Hero_ProjectileLaunch.h"
#include "SFGA_Hero_ProjectileMultiSummon.generated.h"

/**
 * 멀티 소환/발사 어빌리티
 * - 기존 ProjectileLaunch를 상속받아 투사체 설정, 몽타주 재생 로직은 그대로 사용
 * - 발사 로직만 원형 배치 및 다중 발사로 변경
 */
UCLASS()
class SF_API USFGA_Hero_ProjectileMultiSummon : public USFGA_Hero_ProjectileLaunch
{
	GENERATED_BODY()

public:
	USFGA_Hero_ProjectileMultiSummon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	/** * [오버라이드] 
	 * 부모의 단일 발사 로직을 무시하고, 멀티 발사 프로세스를 시작
	 */
	virtual void OnProjectileSpawnEventReceived(FGameplayEventData Payload) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,bool bReplicateEndAbility,bool bWasCancelled) override;

	virtual TArray<FSFPoolPrewarmEntry> GetPoolPrewarmEntries() const override;
	
private:
	/** 개별 투사체를 스폰 (목표 지점을 향해 발사) */
	void SpawnProjectileAt(const FTransform& SpawnTM);

	/** 순차 발사를 위한 타이머 콜백 */
	void OnSequentialSpawnTimer();

	/** 플레이어가 바라보는 지점(Crosshair Hit Point)을 계산하는 헬퍼 함수 */
	FVector GetAimSystemTargetLocation() const;

protected:
	// --- 멀티 발사 설정 ---

	// 발사할 투사체의 총 개수
	UPROPERTY(EditDefaultsOnly, Category = "SF|Projectile|Multi")
	int32 NumProjectiles = 5;

	// 무기(혹은 소켓)를 중심으로 투사체가 생성될 반경 (cm)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Projectile|Multi")
	float SpawnRadius = 150.0f;

	// 배치할 각도 범위 (360 = 완전 원형, 180 = 반원)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Projectile|Multi", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float ArcAngle = 360.0f;

	// 배치 시작 각도 오프셋 (180으로 설정하면 오각형의 평평한 면이 뒤로 가고 꼭짓점이 앞으로 옴)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Projectile|Multi")
	float StartAngleOffset = 180.0f;

	// --- 순차 발사 설정 ---

	// true면 하나씩 순차적으로 발사, false면 동시에 모든 투사체 발사
	UPROPERTY(EditDefaultsOnly, Category = "SF|Projectile|Multi|Timing")
	bool bSpawnSequential = true;

	// 순차 발사 시 투사체 간 생성 간격 (초)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Projectile|Multi|Timing", meta = (EditCondition = "bSpawnSequential"))
	float SpawnInterval = 0.1f;

	// --- 조준(수렴) 설정 ---

	// 조준선(LineTrace) 거리 (cm). 허공을 볼 때 투사체가 모이는 소실점 거리
	UPROPERTY(EditDefaultsOnly, Category = "SF|Projectile|Multi")
	float TraceDistance = 5000.0f;

private:
	// 순차 발사를 위한 타이머 핸들
	FTimerHandle SpawnTimerHandle;

	// 계산 후 정렬(Sorting)된 발사 위치 목록
	TArray<FTransform> SortedSpawnTransforms;

	// 시전 당시 플레이어가 바라보던 목표 지점 (캐싱)
	FVector CachedTargetLocation;

	// 현재 몇 개까지 발사했는지 추적
	int32 CurrentSpawnCount = 0;
};