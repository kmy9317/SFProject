// SFBTTask_RunEQSQuery.h

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "SFBTTask_RunEQSQuery.generated.h"

class UEnvQuery;

/**
 * USFBTTask_RunEQSQuery
 *
 * EQS Query를 실행하고 결과가 올 때까지 대기하는 Task
 * - Service와 달리 EQS 완료를 보장한 후 다음 노드로 진행
 * - 비동기 타이밍 문제 해결
 * - 한 번만 실행되며, 지속적인 업데이트가 필요하면 Service 사용
 */
UCLASS()
class SF_API USFBTTask_RunEQSQuery : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USFBTTask_RunEQSQuery(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	

	/** 실행할 EQS Query 에셋 */
	UPROPERTY(EditAnywhere, Category = "EQS")
	TObjectPtr<UEnvQuery> QueryTemplate;

	/** 결과를 저장할 Blackboard 키 (Vector) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName ResultKeyName = FName("StrafeLocation");

	/** EQS Query 실행 모드 */
	UPROPERTY(EditAnywhere, Category = "EQS")
	TEnumAsByte<EEnvQueryRunMode::Type> RunMode = EEnvQueryRunMode::SingleResult;

	/** NavMesh 검증 활성화 (결과가 NavMesh 위에 있는지 확인) */
	UPROPERTY(EditAnywhere, Category = "EQS")
	bool bProjectToNavMesh = true;

	/** NavMesh 검증 반경 */
	UPROPERTY(EditAnywhere, Category = "EQS", meta = (EditCondition = "bProjectToNavMesh"))
	float NavMeshProjectionRadius = 500.0f;

	/** EQS 실패 시 Task 결과 (Succeeded로 설정하면 실패해도 계속 진행) */
	UPROPERTY(EditAnywhere, Category = "EQS")
	TEnumAsByte<EBTNodeResult::Type> FailureResult = EBTNodeResult::Failed;

protected:
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	// EQS Query 완료 콜백
	void OnQueryFinished(TSharedPtr<FEnvQueryResult> Result);

private:
	/** BehaviorTreeComponent 캐싱 (콜백에서 사용) */
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	/** 현재 실행 중인 쿼리 ID */
	int32 QueryID = INDEX_NONE;
};
