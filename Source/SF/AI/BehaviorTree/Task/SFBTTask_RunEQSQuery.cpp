// SFBTTask_RunEQSQuery.cpp

#include "SFBTTask_RunEQSQuery.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "NavigationSystem.h"
#include "AI/Controller/SFEnemyController.h"
#include "VisualLogger/VisualLogger.h"

USFBTTask_RunEQSQuery::USFBTTask_RunEQSQuery(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = "Run EQS Query";
	bNotifyTaskFinished = true;
	bNotifyTick = false;
}

EBTNodeResult::Type USFBTTask_RunEQSQuery::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{

	if (!QueryTemplate)
	{

		return EBTNodeResult::Failed;
	}

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{

		return EBTNodeResult::Failed;
	}
	ASFEnemyController* EnemyController = Cast<ASFEnemyController>(AIController);
	if (!EnemyController)
	{
		return EBTNodeResult::Failed;
	}
	float CurrentDistance = EnemyController->GetDistanceToTarget();

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}


	UEnvQueryManager* EnvQueryManager = UEnvQueryManager::GetCurrent(AIController->GetWorld());
	if (!EnvQueryManager)
	{

		return EBTNodeResult::Failed;
	}

	CachedOwnerComp = &OwnerComp;

	FEnvQueryRequest QueryRequest(QueryTemplate, AIController);

	
	const float MaxMultiple = 1.5f;
	const float MinMultiple = 0.7f;
	QueryRequest.SetFloatParam(FName("CurrentDistance"), CurrentDistance);
	QueryRequest.SetFloatParam(FName("DistanceMin"), CurrentDistance  * MinMultiple);
	QueryRequest.SetFloatParam(FName("DistanceMax"), CurrentDistance * MaxMultiple);


	QueryID = QueryRequest.Execute(
		RunMode,
		FQueryFinishedSignature::CreateUObject(
			this,
			&USFBTTask_RunEQSQuery::OnQueryFinished
		)
	);

	if (QueryID == INDEX_NONE)
	{
		CachedOwnerComp.Reset();
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

void USFBTTask_RunEQSQuery::OnQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
	if (!OwnerComp)
	{
		return;
	}

	AAIController* AIController = OwnerComp->GetAIOwner();
	EBTNodeResult::Type TaskResult = FailureResult;
	

	if (!Result.IsValid())
	{
		FinishLatentTask(*OwnerComp, TaskResult);
		return;
	}

	if (!Result->IsSuccessful())
	{
		FinishLatentTask(*OwnerComp, TaskResult);
		return;
	}

	if (Result->Items.Num() == 0)
	{
	
		FinishLatentTask(*OwnerComp, TaskResult);
		return;
	}
	FVector ResultLocation = Result->GetItemAsLocation(0);

	if (bProjectToNavMesh)
	{
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(AIController->GetWorld());
		if (NavSys)
		{
			FNavLocation NavLocation;
			const bool bOnNavMesh = NavSys->ProjectPointToNavigation(
				ResultLocation,
				NavLocation,
				FVector(NavMeshProjectionRadius, NavMeshProjectionRadius, NavMeshProjectionRadius)
			);

			if (bOnNavMesh)
			{
				ResultLocation = NavLocation.Location;
			}
			else
			{
				FinishLatentTask(*OwnerComp, TaskResult);
				return;
			}
		}
	}

	UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	if (BlackboardComp && ResultKeyName != NAME_None)
	{
		BlackboardComp->SetValueAsVector(ResultKeyName, ResultLocation);

		TaskResult = EBTNodeResult::Succeeded;
	}
	else
	{
		TaskResult = EBTNodeResult::Failed;
	}

	FinishLatentTask(*OwnerComp, TaskResult);
}

void USFBTTask_RunEQSQuery::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{

	// 실행 중인 쿼리가 있으면 중단
	if (QueryID != INDEX_NONE)
	{
		UEnvQueryManager* EnvQueryManager = UEnvQueryManager::GetCurrent(OwnerComp.GetWorld());
		if (EnvQueryManager)
		{
			EnvQueryManager->AbortQuery(QueryID);
		}
		QueryID = INDEX_NONE;
	}

	// 캐시 정리
	CachedOwnerComp.Reset();

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

