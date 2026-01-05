// SF/AI/BehaviorTree/Service/BTService_UpdateTarget.h

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateTarget.generated.h"

/**
 * [통합 타겟팅 서비스]
 * - 일반 몬스터: 시야 기반 추적, 죽으면 해제.
 * - 보스 몬스터: 거리 정보(Distance) 실시간 갱신 (패턴용), 공격 중에도 거리 갱신.
 */
UCLASS()
class SF_API UBTService_UpdateTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateTarget();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// === [Output] Blackboard Keys ===
	
	/** 타겟 액터 (Object) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	/** 타겟 유무 (Bool) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector HasTargetKey;

	/** [보스 필수] 타겟과의 거리 (Float) - 보스 패턴 조건(Decorator)에서 사용 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector DistanceToTargetKey;

	/** 마지막 위치 (Vector) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector LastKnownPositionKey;

	// === [Config] 설정 값 ===

	/** 타겟 교체 임계값 (0.0 = 거리 조금이라도 가까우면 즉시 교체) */
	UPROPERTY(EditAnywhere, Category = "Target Priority", meta = (ClampMin = "0.0"))
	float ScoreDifferenceThreshold = 0.0f;

	/** 최대 추격 거리 (보스는 에디터에서 999999로 설정 권장) */
	UPROPERTY(EditAnywhere, Category = "Target Priority", meta = (ClampMin = "0.0"))
	float MaxChaseDistance = 2000.0f;

	/** 감지할 태그 (Player) */
	UPROPERTY(EditAnywhere, Category = "Target Priority")
	FName TargetTag = FName("Player");

private:
	/** 거리 기반 점수 계산 */
	float CalculateTargetScore(APawn* MyPawn, AActor* Target) const;

	/** 타겟 상태(죽음/무적) 확인 */
	bool IsTargetValid(AActor* TargetActor) const;
};