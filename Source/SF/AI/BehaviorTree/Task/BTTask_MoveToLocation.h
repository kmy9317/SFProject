// BTTask_MoveToLocation.h
// Blackboard Vector 변경 감지하는 MoveTo Task

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_MoveToLocation.generated.h"

/**
 * BTTask_MoveToLocation
 *
 * Blackboard의 Vector 키를 실시간으로 추적하는 MoveTo Task
 * - Service가 위치를 업데이트하면 자동으로 경로 재계산
 * - [중요] NodeMemory를 사용하여 멀티 AI 환경에서 안전하게 동작
 */
UCLASS()
class SF_API UBTTask_MoveToLocation : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_MoveToLocation();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	// ========================================
	// 메모리 관리 (NodeMemory)
	// ========================================
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;

	// ========================================
	// 설정
	// ========================================

	/** 목표 지점 도달 판정 거리 */
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 50.f;

	/** 경로 재계산 최소 거리 (이전 목표와 새 목표 차이) */
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0"))
	float RepathDistance = 100.f;

private:
	/**
	 * NodeMemory 구조체
	 * 멤버 변수 대신 사용하여 각 AI 인스턴스별 데이터 분리
	 */
	struct FBTMoveToLocationMemory
	{
		FVector CurrentGoalLocation;
	};
};