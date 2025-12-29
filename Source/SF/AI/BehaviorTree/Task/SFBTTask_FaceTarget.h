#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "SFBTTask_FaceTarget.generated.h"

UCLASS()
class SF_API USFBTTask_FaceTarget : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	USFBTTask_FaceTarget();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	AActor* GetTargetFromBlackboard(UBehaviorTreeComponent& OwnerComp) const;

public:
	// 허용 각도
	UPROPERTY(EditAnywhere, Category = "Rotation", meta = (ClampMin = "1.0", ClampMax = "45.0"))
	float AcceptableAngle = 10.0f;
};
