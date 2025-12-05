// SFBTTask_RunEQSQuery.cpp

#include "SFBTTask_RunEQSQuery.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "NavigationSystem.h"
#include "AI/Controller/SFEnemyController.h"

USFBTTask_RunEQSQuery::USFBTTask_RunEQSQuery(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = "Run EQS Query";
	bNotifyTaskFinished = true;
	bNotifyTick = false;

    // [핵심 수정] 이 옵션이 없으면 10마리 AI가 변수를 공유하여 서로 멈추게 됩니다.
	bCreateNodeInstance = true;
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

    // 파라미터 설정용 (필요시 확장 가능)
	ASFEnemyController* EnemyController = Cast<ASFEnemyController>(AIController);
	float CurrentDistance = 0.0f;
	if (EnemyController)
	{
		if (AActor* Target = EnemyController->TargetActor)
		{
			if (APawn* Pawn = EnemyController->GetPawn())
			{
				CurrentDistance = FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation());
			}
		}
	}

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

    // 파라미터 전달
	const float MaxMultiple = 1.5f;
	const float MinMultiple = 0.7f;
	QueryRequest.SetFloatParam(FName("CurrentDistance"), CurrentDistance);
	QueryRequest.SetFloatParam(FName("DistanceMin"), CurrentDistance * MinMultiple);
	QueryRequest.SetFloatParam(FName("DistanceMax"), CurrentDistance * MaxMultiple);

    // 쿼리 실행
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

//AbortTask: 다른 태스크로 전환될 때 실행 중인 쿼리 취소
EBTNodeResult::Type USFBTTask_RunEQSQuery::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (QueryID != INDEX_NONE)
	{
		UEnvQueryManager* EnvQueryManager = UEnvQueryManager::GetCurrent(OwnerComp.GetWorld());
		if (EnvQueryManager)
		{
			EnvQueryManager->AbortQuery(QueryID);
		}
		QueryID = INDEX_NONE;
	}

	return Super::AbortTask(OwnerComp, NodeMemory);
}

void USFBTTask_RunEQSQuery::OnQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
    // 이미 태스크가 종료되었거나 컴포넌트가 유효하지 않으면 무시
	if (!CachedOwnerComp.IsValid())
    {
        return;
    }

	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
    QueryID = INDEX_NONE; // 쿼리 종료

	EBTNodeResult::Type TaskResult = FailureResult;

	if (!Result.IsValid() || !Result->IsSuccessful() || Result->Items.Num() == 0)
	{
		FinishLatentTask(*OwnerComp, TaskResult);
		return;
	}

	FVector ResultLocation = Result->GetItemAsLocation(0);
    AAIController* AIController = OwnerComp->GetAIOwner();

	if (bProjectToNavMesh && AIController)
	{
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(AIController->GetWorld());
		if (NavSys)
		{
			FNavLocation NavLocation;
            // [보완] Z축 범위를 넉넉하게(500) 잡아서 언덕 위/아래 포인트도 잘 잡히게 수정
			const bool bOnNavMesh = NavSys->ProjectPointToNavigation(
				ResultLocation,
				NavLocation,
				FVector(NavMeshProjectionRadius, NavMeshProjectionRadius, 500.0f) 
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
	// 아직 쿼리가 돌고 있는데 태스크가 끝났다면(Abort 등) 쿼리 취소
	if (QueryID != INDEX_NONE)
	{
		UEnvQueryManager* EnvQueryManager = UEnvQueryManager::GetCurrent(OwnerComp.GetWorld());
		if (EnvQueryManager)
		{
			EnvQueryManager->AbortQuery(QueryID);
		}
		QueryID = INDEX_NONE;
	}

	CachedOwnerComp.Reset();

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}