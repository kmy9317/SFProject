// SF/AI/BehaviorTree/Service/BTService_UpdateTarget.h

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateTarget.generated.h"

class ASFEnemyController;

/**
 * 타겟 감지 및 거리 기반 타겟 관리 서비스
 */
UCLASS()
class SF_API UBTService_UpdateTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateTarget();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** Blackboard Key: TargetActor */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	/** Blackboard Key: bHasTarget */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector HasTargetKey;

	/** 타겟 전환 점수 차이 임계값 */
	UPROPERTY(EditAnywhere, Category = "Target Priority", meta = (ClampMin = "0.0"))
	float ScoreDifferenceThreshold = 50.f;

	/** * [수정] 최대 추격 거리
	 * 이 값이 시야(Perception)보다 작으면 타겟이 계속 끊깁니다.
	 * 기본값을 999999로 설정하여 사실상 무제한으로 만듭니다.
	 */
	UPROPERTY(EditAnywhere, Category = "Target Priority", meta = (ClampMin = "0.0"))
	float MaxChaseDistance = 999999.0f; // 기존 2000.0f -> 999999.0f

private:
	float CalculateTargetScore(UBehaviorTreeComponent& OwnerComp, AActor* Target,class ASFBaseAIController* AIController) const;
};