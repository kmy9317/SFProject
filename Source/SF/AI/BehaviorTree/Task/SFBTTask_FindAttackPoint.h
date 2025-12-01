// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "SFBTTask_FindAttackPoint.generated.h"

class UEnvQuery;

/**
 * USFBTTask_FindAttackPoint
 * 
 * Ability 기반 공격 위치를 EQS로 찾는 Task
 * - Ability에서 MinDistance, MaxDistance 자동 추출
 * - EQS 완료를 보장한 후 Success/Failed 반환
 * - Service와 달리 한 번만 실행
 */
UCLASS()
class SF_API USFBTTask_FindAttackPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USFBTTask_FindAttackPoint(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	/** 타겟 Actor 키 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName TargetActorKeyName = FName("TargetActor");

	/** Ability Tag 키 (Blackboard에서 읽어옴) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName AbilityTagKeyName = FName("SelectedAbilityTag");

	/** NavMesh 검증 활성화 */
	UPROPERTY(EditAnywhere, Category = "EQS")
	bool bProjectToNavMesh = true;

	/** NavMesh 검증 반경 */
	UPROPERTY(EditAnywhere, Category = "EQS", meta = (EditCondition = "bProjectToNavMesh"))
	float NavMeshProjectionRadius = 500.0f;

	/** EQS 실패 시 Task 결과 */
	UPROPERTY(EditAnywhere, Category = "EQS")
	TEnumAsByte<EBTNodeResult::Type> FailureResult = EBTNodeResult::Failed;

	/** 이미 공격 범위 안에 있으면 EQS 스킵 */
	UPROPERTY(EditAnywhere, Category = "EQS")
	bool bSkipIfInRange = true;

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

