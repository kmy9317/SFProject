#include "BTD_GuardSlotAvailable.h"

#include "AI/Controller/SFEnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BTNode.h"
#include "AI/SFCombatSlotManager.h"

UBtd_GuardSlotAvailable::UBtd_GuardSlotAvailable()
{
	NodeName = "Guard Slot Available"; // [변경] 노드 이름 변경

	// ⭐ Observer Aborts: Both
	// 슬롯 상태가 바뀌면 즉시 현재 행동을 중단하거나, 다시 진입하도록 설정
	FlowAbortMode = EBTFlowAbortMode::Both;

	// ⭐ Tick 활성화
	bNotifyTick = true;
}

bool UBtd_GuardSlotAvailable::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	// [변경] 캐스팅 대상을 SFEnemyController로 변경
	ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("[SFBTD_GuardSlotAvailable] AIController is NULL or not ASFEnemyController!"));
		return false;
	}

	// ⭐ 멀티플레이 대응: 클라이언트는 서버가 복제해준 변수값 사용
	UWorld* World = AIController->GetWorld();
	if (World && World->GetNetMode() == NM_Client)
	{
		// [변경] 변수명 매핑: bHasAttackSlot -> bHasGuardSlot
		return AIController->bHasGuardSlot;
	}

	// ⭐ 서버 로직 시작
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return false;
	}

	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject("TargetActor"));
	
	// CombatSlotManager 가져오기
	USFCombatSlotManager* Manager = World->GetSubsystem<USFCombatSlotManager>();
	if (!Manager)
	{
		UE_LOG(LogTemp, Error, TEXT("[SFBTD_GuardSlotAvailable] CombatSlotManager is NULL!"));
		return false;
	}

	// 타겟이 없으면 슬롯 해제 후 false 반환
	if (!TargetActor)
	{
		Manager->ReleaseSlot(AIController);
		return false;
	}

	// ========================================
	// 슬롯 매니저를 통한 슬롯 확인 및 요청
	// ========================================
	
	// 이미 슬롯을 보유하고 있는지 확인
	if (Manager->HasSlot(AIController, TargetActor))
	{
		// [추가 안전장치] 매니저는 있다고 하는데 컨트롤러 변수가 false라면 동기화
		if (!AIController->bHasGuardSlot)
		{
			AIController->bHasGuardSlot = true;
		}
		return true;
	}

	// 새로운 슬롯 요청
	// (Manager 내부에서 SFEnemyController를 처리할 수 있도록 Manager도 수정되어야 함)
	const bool bGranted = Manager->RequestSlot(AIController, TargetActor);
	
	// [중요] 요청 결과에 따라 컨트롤러 변수 업데이트 (서버 권한)
	// SFEnemyController.h에 bHasGuardSlot이 Replicated로 되어 있으므로 여기서 설정하면 클라로 전파됨
	AIController->bHasGuardSlot = bGranted;

	return bGranted;
}

void UBtd_GuardSlotAvailable::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	Super::InitializeMemory(OwnerComp, NodeMemory, InitType);

	FSFGuardSlotMemory* Memory = reinterpret_cast<FSFGuardSlotMemory*>(NodeMemory);
	if (Memory)
	{
		ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
		FString PawnName = AIController && AIController->GetPawn() ? AIController->GetPawn()->GetName() : TEXT("Unknown");

		// [로그] 이름 변경 반영
		UE_LOG(LogTemp, Verbose, TEXT("[SFBTD_GuardSlotAvailable] 🎬 %s: InitializeMemory"), *PawnName);

		Memory->bLastResult = false;
		Memory->bInitialized = false;
	}
}

void UBtd_GuardSlotAvailable::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FSFGuardSlotMemory* Memory = reinterpret_cast<FSFGuardSlotMemory*>(NodeMemory);
	if (!Memory)
	{
		return;
	}

	// 현재 상태 계산
	const bool bCurrentResult = CalculateRawConditionValue(OwnerComp, NodeMemory);

	// 상태 변경 감지 시 재평가 요청
	if (!Memory->bInitialized || Memory->bLastResult != bCurrentResult)
	{
		ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
		FString PawnName = AIController && AIController->GetPawn() ? AIController->GetPawn()->GetName() : TEXT("Unknown");

		if (Memory->bInitialized)
		{
			UE_LOG(LogTemp, Log, TEXT("[SFBTD_GuardSlotAvailable] %s: Slot Changed: %s → %s (RequestExecution)"),
				*PawnName,
				Memory->bLastResult ? TEXT("HasSlot") : TEXT("NoSlot"),
				bCurrentResult ? TEXT("HasSlot") : TEXT("NoSlot"));
		}
		else
		{
			Memory->bInitialized = true;
		}

		Memory->bLastResult = bCurrentResult;
		
		// 트리 재평가 (Decorator 조건이 바뀌었으니 실행 흐름 변경)
		OwnerComp.RequestExecution(this);
	}
}

void UBtd_GuardSlotAvailable::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);

	ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		return;
	}

	// 이 노드가 더 이상 유효하지 않게 되면(트리 분기 탈출 등) 슬롯 해제 시도
	UWorld* World = AIController->GetWorld();
	if (World)
	{
		USFCombatSlotManager* Manager = World->GetSubsystem<USFCombatSlotManager>();
		if (Manager)
		{
			Manager->ReleaseSlot(AIController);
			
			// [중요] 변수 상태 동기화
			AIController->bHasGuardSlot = false;
		}
	}
}