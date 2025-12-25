#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "SFBTTask_FaceTarget.generated.h"

class ASFBaseAIController;

UCLASS()
class SF_API USFBTTask_FaceTarget : public UBTTask_BlackboardBase
{
	GENERATED_BODY()


public:
	USFBTTask_FaceTarget();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	// ❌ 기존: ActorForward 기준 (TurnInPlace 중 부정확)
	float CalculateAngleToTarget(APawn* InPawn, AActor* InTarget, UBehaviorTreeComponent& OwnerComp);

	// ✅ 신규: ControlRotation 기준 (논리적 회전 완료 판단)
	float CalculateAngleToTarget_Control(APawn* InPawn, AActor* InTarget, AAIController* AIC);

	void SyncControlRotationToPawn(ASFBaseAIController* AIC);

public:
	// Blackboard 키로 각도를 직접 받을 수 있는 옵션
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector AngleKey;
    
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	bool bUseAngleKey = false;

	// 회전 완료로 판단할 각도 (이 값 이하면 Task 성공)
	UPROPERTY(EditAnywhere, Category = "Rotation", meta = (ClampMin = "1.0", ClampMax = "45.0"))
	float AcceptableAngle = 10.0f;

	// TurnInPlace를 사용할 최소 각도 (이 값 이상이면 TurnInPlace 사용)
	UPROPERTY(EditAnywhere, Category = "Rotation", meta = (ClampMin = "30.0", ClampMax = "120.0"))
	float TurnInPlaceThreshold = 70.0f;

	// TurnInPlace에서 ControllerYaw로 전환할 각도
	// (회전 중 각도가 이 값 이하로 줄어들면 부드러운 회전으로 전환)
	UPROPERTY(EditAnywhere, Category = "Rotation", meta = (ClampMin = "10.0", ClampMax = "60.0"))
	float SmoothRotationThreshold = 30.0f;

	// Task 실행 시 목표 방향으로 ControlRotation을 즉시 설정할지 여부
	UPROPERTY(EditAnywhere, Category = "Rotation")
	bool bSetControlRotationToTarget = true;
};