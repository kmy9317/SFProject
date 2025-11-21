// BTTask_MoveToLocation.cpp

#include "BTTask_MoveToLocation.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "AI/Controller/SFEnemyController.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_MoveToLocation::UBTTask_MoveToLocation()
{
	NodeName = "Move To Location (Dynamic)";

	// Tick 활성화 (Blackboard 변경 감지용)
	bNotifyTick = true;
	bNotifyTaskFinished = true;

	// Blackboard 키는 Vector 타입만 허용
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_MoveToLocation, BlackboardKey));
}

// NodeMemory 크기 반환
uint16 UBTTask_MoveToLocation::GetInstanceMemorySize() const
{
	return sizeof(FBTMoveToLocationMemory);
}

// NodeMemory 초기화
void UBTTask_MoveToLocation::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	Super::InitializeMemory(OwnerComp, NodeMemory, InitType);

	FBTMoveToLocationMemory* MyMemory = new (NodeMemory) FBTMoveToLocationMemory();
	MyMemory->CurrentGoalLocation = FVector::ZeroVector;
}

EBTNodeResult::Type UBTTask_MoveToLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// [변경] SFEnemyController 사용
	ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();

	if (!AIController || !BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	// Blackboard에서 목표 위치 읽기
	const FVector TargetLocation = BlackboardComp->GetValueAsVector(BlackboardKey.SelectedKeyName);

	// 유효성 체크 (Zero Vector면 실패 처리)
	if (TargetLocation.IsZero())
	{
		// UE_LOG(LogTemp, Warning, TEXT("[BTTask_MoveToLocation] Target is Zero"));
		return EBTNodeResult::Failed;
	}

	// NodeMemory에 현재 목표 저장
	FBTMoveToLocationMemory* MyMemory = reinterpret_cast<FBTMoveToLocationMemory*>(NodeMemory);
	MyMemory->CurrentGoalLocation = TargetLocation;

	// 이동 시작
	const EPathFollowingRequestResult::Type Result = AIController->MoveToLocation(
		TargetLocation,
		AcceptanceRadius,
		true,  // bStopOnOverlap
		true,  // bUsePathfinding
		false, // bProjectDestinationToNavigation
		true,  // bCanStrafe
		nullptr, // FilterClass
		true   // bAllowPartialPath
	);

	if (Result == EPathFollowingRequestResult::RequestSuccessful)
	{
		return EBTNodeResult::InProgress;
	}
	else if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}

void UBTTask_MoveToLocation::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();

	if (!AIController || !BlackboardComp)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// ========================================
	// 1. 이동 완료 체크
	// ========================================
	UPathFollowingComponent* PathFollowing = AIController->GetPathFollowingComponent();
	if (PathFollowing)
	{
		if (PathFollowing->GetStatus() == EPathFollowingStatus::Idle)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
		else if (PathFollowing->GetStatus() != EPathFollowingStatus::Moving)
		{
			// Moving도 아니고 Idle도 아니면 실패 (Paused 등)
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}
	}

	// ========================================
	// 2. Blackboard 위치 변경 감지 (Repath)
	// ========================================
	const FVector NewTargetLocation = BlackboardComp->GetValueAsVector(BlackboardKey.SelectedKeyName);
	
	// NodeMemory에서 이전 목표 가져오기
	FBTMoveToLocationMemory* MyMemory = reinterpret_cast<FBTMoveToLocationMemory*>(NodeMemory);
	const float DistanceChanged = FVector::Dist(MyMemory->CurrentGoalLocation, NewTargetLocation);

	if (DistanceChanged > RepathDistance)
	{
		// 목표 위치 업데이트 및 이동 명령 재전송
		MyMemory->CurrentGoalLocation = NewTargetLocation;

		AIController->MoveToLocation(
			NewTargetLocation,
			AcceptanceRadius,
			true, true, false, true, nullptr, true
		);
	}
}

void UBTTask_MoveToLocation::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
	if (AIController)
	{
		// 태스크가 끝나면(성공/실패/중단) 이동 정지
		AIController->StopMovement();
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}