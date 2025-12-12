// BTService_UpdateTarget.h

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateTarget.generated.h"

class ASFEnemyController;

/**
 * UBTService_UpdateTarget (수정됨)
 * - 기능 1: 타겟 탐색 (Perception) 및 선정 (Score)
 * - 기능 2: 타겟 유지 (Distance 기반 - 히스테리시스 적용)
 * - 기능 3: 전투 슬롯(Slot) 요청 및 유지
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

	/** 이 거리보다 가까우면 슬롯 제한을 무시하고 강제 공격권 획득 */
	UPROPERTY(EditAnywhere, Category = "Combat", meta = (ClampMin = "0.0"))
	float ForceAttackDistance = 200.0f;

	/** [추가] 최대 추격 거리. 이 거리를 벗어나면 타겟을 포기합니다. (시야보다 넓어야 함) */
	UPROPERTY(EditAnywhere, Category = "Target Priority", meta = (ClampMin = "0.0"))
	float MaxChaseDistance = 2000.0f;

private:
	/** 점수 계산 함수 */
	float CalculateTargetScore(UBehaviorTreeComponent& OwnerComp, AActor* Target, ASFEnemyController* AIController) const;
};