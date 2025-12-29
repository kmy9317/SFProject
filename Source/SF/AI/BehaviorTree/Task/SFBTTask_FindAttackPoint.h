#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "SFBTTask_FindAttackPoint.generated.h"

class UEnvQuery;

UCLASS()
class SF_API USFBTTask_FindAttackPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USFBTTask_FindAttackPoint(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

private:
	void OnQueryFinished(TSharedPtr<FEnvQueryResult> Result);

public:
	UPROPERTY(EditAnywhere, Category = "EQS")
	TObjectPtr<UEnvQuery> QueryTemplate;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName ResultKeyName = TEXT("StrafeLocation");

	UPROPERTY(EditAnywhere, Category = "EQS")
	TEnumAsByte<EEnvQueryRunMode::Type> RunMode = EEnvQueryRunMode::SingleResult;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName TargetActorKeyName = TEXT("TargetActor");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName AbilityTagKeyName = TEXT("SelectedAbilityTag");

	UPROPERTY(EditAnywhere, Category = "EQS")
	bool bProjectToNavMesh = true;

	UPROPERTY(EditAnywhere, Category = "EQS", meta = (EditCondition = "bProjectToNavMesh"))
	float NavMeshProjectionRadius = 500.f;

	UPROPERTY(EditAnywhere, Category = "EQS")
	TEnumAsByte<EBTNodeResult::Type> FailureResult = EBTNodeResult::Failed;

	UPROPERTY(EditAnywhere, Category = "EQS")
	bool bSkipIfInRange = true;

private:
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
	int32 QueryID = INDEX_NONE;
};
