// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/Skill/SkillActor/SFGroundAOE.h"
#include "SFVacuumGroundAOE.generated.h"

/**
 * 기본 장판 기능 + 범위 내 적을 중앙으로 끌어당기는 진공 효과(Vacuum) 추가
 */
UCLASS()
class SF_API ASFVacuumGroundAOE : public ASFGroundAOE
{
	GENERATED_BODY()
	
public:
	ASFVacuumGroundAOE(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 끌어당길 수 있는 대상인지 확인 (아군 제외, 사망자 제외 등)
	bool IsValidPullTarget(AActor* TargetActor) const;

protected:
	// === 설정 요소 === //

	// 적을 끌어당기는 힘 (초당 이동 속도에 영향)
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Vacuum")
	float PullStrength = 350.f;

	// 중앙에서 이 거리 이내에 들어오면 더 이상 끌어당기지 않음 (떨림 방지)
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Vacuum")
	float PullDeadZoneRadius = 50.f;

	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Vacuum")
	float PullHeightOffset = 100.f;
	
	// 끌어당기기 로직 수행 주기 (0이면 매 프레임 Tick, 값이 있으면 Timer 사용)
	// 성능 최적화를 위해 Tick 대신 0.05~0.1초 간격으로 힘을 가하고 싶을 때 사용
	UPROPERTY(EditDefaultsOnly, Category="SF|AOE|Vacuum")
	float PullInterval = 0.0f;
};