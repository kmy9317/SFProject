#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateTarget.generated.h"

class ASFEnemyController; // SF 컨트롤러 (파일 이름은 이거라고 가정)

/**
 * BTService_UpdateTarget
 *
 * - 주기적으로(0.2초) 주변 적을 감지하고 점수를 계산
 * - 가장 가까운 적을 추적
 * - CombatSlotManager(접두어 없음)를 통해 공격/가드 슬롯 요청
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
	float ScoreDifferenceThreshold = 0.f;

private:
	/** 점수 계산 함수 */
	float CalculateTargetScore(UBehaviorTreeComponent& OwnerComp, AActor* Target, ASFEnemyController* AIController) const;
};