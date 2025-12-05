// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateTarget.generated.h"

class ASFEnemyController;

/**
 * UBTService_UpdateTarget (통합 완전체 버전)
 * * [기능 목록]
 * 1. 타겟 탐색 (Perception) - 기존 기능 복구
 * 2. 최적 타겟 선정 (Score) - 기존 기능 복구
 * 3. 슬롯 요청 및 유지 (Request/Maintain) - 삭제됐던 기능 복구 및 강화
 * 4. 근접 강제 공격 (Force Attack) - 신규 기능 추가
 */
UCLASS()
class SF_API UBTService_UpdateTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateTarget();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** Blackboard Key: TargetActor */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	/** Blackboard Key: bHasTarget */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector HasTargetKey;

	/** 타겟 전환 임계값 (점수 차이) */
	UPROPERTY(EditAnywhere, Category = "Target Priority", meta = (ClampMin = "0.0"))
	float ScoreDifferenceThreshold = 50.f;

	/** [신규] 이 거리보다 가까우면 슬롯 제한을 무시하고 강제로 공격권을 따냅니다. (cm) */
	UPROPERTY(EditAnywhere, Category = "Combat", meta = (ClampMin = "0.0"))
	float ForceAttackDistance = 200.0f;

private:
	/** 점수 계산 함수 */
	float CalculateTargetScore(UBehaviorTreeComponent& OwnerComp, AActor* Target, ASFEnemyController* AIController) const;
};