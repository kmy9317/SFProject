// BTService_RunEQSQuery.cpp

#include "BTService_RunEQSQuery.h"
#include "AI/Controller/SFEnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshPath.h"

UBTService_RunEQSQuery::UBTService_RunEQSQuery()
{
	NodeName = "Run EQS Query (Dynamic)";
	Interval = 1.0f;
	RandomDeviation = 0.2f;
}

uint16 UBTService_RunEQSQuery::GetInstanceMemorySize() const
{
	return sizeof(FBTRunEQSQueryMemory);
}

void UBTService_RunEQSQuery::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	Super::InitializeMemory(OwnerComp, NodeMemory, InitType);

	FBTRunEQSQueryMemory* MyMemory = new (NodeMemory) FBTRunEQSQueryMemory();
	MyMemory->LastTargetActor = nullptr;
    MyMemory->QueryID = INDEX_NONE;
}

UBTService_RunEQSQuery::FBTRunEQSQueryMemory::FBTRunEQSQueryMemory(): LastTargetActor(nullptr), QueryID(INDEX_NONE)
{
}

void UBTService_RunEQSQuery::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    //ASFEnemyController 캐스팅
	ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();

	if (!AIController || !BlackboardComp) return;

	if (!QueryTemplate)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTService] QueryTemplate is null!"));
		return;
	}

    FBTRunEQSQueryMemory* MyMemory = reinterpret_cast<FBTRunEQSQueryMemory*>(NodeMemory);

	// ========================================
	// 타겟 변경 감지 (즉시 실행 로직)
	// ========================================
	bool bForceRunQuery = false;

	if (bDetectTargetChange)
	{
		AActor* CurrentTarget = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKeyName));

		// 이전 타겟과 다르면 강제 실행 플래그 설정
		if (CurrentTarget != MyMemory->LastTargetActor.Get())
		{
			bForceRunQuery = true;
			MyMemory->LastTargetActor = CurrentTarget;

            FString PawnName = AIController->GetPawn() ? AIController->GetPawn()->GetName() : TEXT("Unknown");
			UE_LOG(LogTemp, Log, TEXT("[BTService] %s: Target Changed -> Force Run EQS"), *PawnName);
		}
	}

	// ========================================
	// EQS Query 실행
	// ========================================
	UEnvQueryManager* QueryManager = UEnvQueryManager::GetCurrent(AIController->GetWorld());
	if (!QueryManager) return;

	// 이전 쿼리 처리 로직 
	if (MyMemory->QueryID != INDEX_NONE)
	{
		// 타겟이 변경된 경우에만 쿼리 중단 후 재실행
		if (bForceRunQuery)
		{
			QueryManager->AbortQuery(MyMemory->QueryID);
			MyMemory->QueryID = INDEX_NONE;
		}
		else
		{
			// 이전 쿼리가 아직 실행 중이면 대기
			return;
		}
	}

	FEnvQueryRequest QueryRequest(QueryTemplate, AIController);

	// 쿼리 실행 및 ID 저장
	MyMemory->QueryID = QueryRequest.Execute(
		RunMode,
		FQueryFinishedSignature::CreateUObject(
			this,
			&UBTService_RunEQSQuery::OnQueryFinished,
			&OwnerComp
		)
	);
}

void UBTService_RunEQSQuery::OnQueryFinished(TSharedPtr<FEnvQueryResult> Result, UBehaviorTreeComponent* OwnerComp)
{
    if (!OwnerComp) return;
	if (!Result.IsValid()) return;

	UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	if (!BlackboardComp) return;

	// ✅ NodeMemory에서 QueryID 가져오기 (언리얼 엔진 표준 방식)
	FBTRunEQSQueryMemory* MyMemory = CastInstanceNodeMemory<FBTRunEQSQueryMemory>(
		OwnerComp->GetNodeMemory(this, OwnerComp->FindInstanceContainingNode(this))
	);

	if (Result->IsSuccessful())
	{
		FVector ResultLocation = Result->GetItemAsLocation(0);

		// ========================================
		// NavMesh 검증 (안전 장치)
		// ========================================
		AAIController* AIController = OwnerComp->GetAIOwner();
		if (AIController)
		{
			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(AIController->GetWorld());
			if (NavSys)
			{
				FNavLocation NavLoc;
				// 500 유닛 반경 내에서 NavMesh 위 좌표 찾기
				const bool bOnNavMesh = NavSys->ProjectPointToNavigation(
					ResultLocation,
					NavLoc,
					FVector(500.f, 500.f, 500.f)
				);

				if (bOnNavMesh)
				{
					ResultLocation = NavLoc.Location;
					BlackboardComp->SetValueAsVector(ResultKeyName, ResultLocation);
				}
			}
			else
			{
                // NavSys가 없으면 검증 없이 저장
				BlackboardComp->SetValueAsVector(ResultKeyName, ResultLocation);
			}
		}
	}

	// 쿼리 완료 후 ID 초기화 
	MyMemory->QueryID = INDEX_NONE;
}