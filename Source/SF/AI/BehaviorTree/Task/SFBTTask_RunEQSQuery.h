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
 * [수정 사항]
 * - bCreateNodeInstance 활성화 (멀티 AI 버그 수정)
 * - AbortTask 구현 (중단 시 EQS 취소)
 */
UCLASS()
class SF_API USFBTTask_RunEQSQuery : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USFBTTask_RunEQSQuery(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// [필수 추가] 태스크 중단 시 실행 중인 쿼리 취소
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** 실행할 EQS Query 에셋 */
	UPROPERTY(EditAnywhere, Category = "EQS")
	TObjectPtr<UEnvQuery> QueryTemplate;

	/** 결과를 저장할 Blackboard 키 (Vector) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName ResultKeyName = FName("StrafeLocation");

	/** EQS Query 실행 모드 */
	UPROPERTY(EditAnywhere, Category = "EQS")
	TEnumAsByte<EEnvQueryRunMode::Type> RunMode = EEnvQueryRunMode::SingleResult;

	/** NavMesh 검증 활성화 */
	UPROPERTY(EditAnywhere, Category = "EQS")
	bool bProjectToNavMesh = true;

	/** NavMesh 검증 반경 */
	UPROPERTY(EditAnywhere, Category = "EQS", meta = (EditCondition = "bProjectToNavMesh"))
	float NavMeshProjectionRadius = 500.0f;

	/** EQS 실패 시 Task 결과 */
	UPROPERTY(EditAnywhere, Category = "EQS")
	TEnumAsByte<EBTNodeResult::Type> FailureResult = EBTNodeResult::Failed;

protected:
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	// EQS Query 완료 콜백
	void OnQueryFinished(TSharedPtr<FEnvQueryResult> Result);

private:
	/** BehaviorTreeComponent 캐싱 */
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	/** 현재 실행 중인 쿼리 ID */
	int32 QueryID = INDEX_NONE;
};