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

	// [헬퍼 함수] 어빌리티의 IsWithinAttackAngle 호출
	bool CheckAbilityAttackAngle(UBehaviorTreeComponent& OwnerComp, AActor* Target) const;

public:
	// 어빌리티를 못 찾았을 때 사용할 기본 오차 (백업용)
	UPROPERTY(EditAnywhere, Category = "AI")
	float DefaultPrecision = 10.0f;

	// [필수] 블랙보드에서 읽어올 어빌리티 태그 키 (예: SelectedAbilityTag)
	UPROPERTY(EditAnywhere, Category = "AI")
	FBlackboardKeySelector AbilityTagKey;
};