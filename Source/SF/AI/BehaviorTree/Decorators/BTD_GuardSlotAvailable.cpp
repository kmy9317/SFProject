// Fill out your copyright notice in the Description page of Project Settings.

#include "BTD_GuardSlotAvailable.h"

#include "AI/Controller/SFEnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BTNode.h"
// #include "AI/SFCombatSlotManager.h" // [제거] 매니저 직접 접근 불필요

UBtd_GuardSlotAvailable::UBtd_GuardSlotAvailable()
{
	NodeName = "Guard Slot Available (Check Only)"; 

	// [중요] Observer Aborts: Lower Priority
	// 슬롯을 잃으면(bHasGuardSlot = false가 되면) 즉시 하위 노드(공격 등) 실행을 중단하고 탈출
	FlowAbortMode = EBTFlowAbortMode::LowerPriority;

	// 실시간 상태 감지를 위해 Tick 활성화
	bNotifyTick = true;
}

bool UBtd_GuardSlotAvailable::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	// SFEnemyController 캐스팅
	ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		return false;
	}

	// [핵심 수정] 
	// 기존: Manager->RequestSlot() 호출 (직접 요청)
	// 변경: AIController->bHasGuardSlot 확인 (단순 검사)
	//
	// 실제 슬롯 요청은 이제 BTService_UpdateTarget에서 타겟을 보고 있는 동안 지속적으로 수행됩니다.
	return AIController->bHasGuardSlot;
}

void UBtd_GuardSlotAvailable::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	Super::InitializeMemory(OwnerComp, NodeMemory, InitType);

	FSFGuardSlotMemory* Memory = reinterpret_cast<FSFGuardSlotMemory*>(NodeMemory);
	if (Memory)
	{
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

	// 현재 상태 계산 (bHasGuardSlot 변수 체크)
	const bool bCurrentResult = CalculateRawConditionValue(OwnerComp, NodeMemory);

	// 상태 변경 감지 시 (있음 <-> 없음) 트리 재평가 요청
	if (!Memory->bInitialized || Memory->bLastResult != bCurrentResult)
	{
		// [로그] 디버깅 필요시 주석 해제
		/*
		ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
		FString PawnName = AIController && AIController->GetPawn() ? AIController->GetPawn()->GetName() : TEXT("Unknown");
		UE_LOG(LogTemp, Log, TEXT("[SFBTD_GuardSlotAvailable] %s: Slot Changed -> %s"), 
			*PawnName, bCurrentResult ? TEXT("HasSlot") : TEXT("LostSlot"));
		*/

		Memory->bInitialized = true;
		Memory->bLastResult = bCurrentResult;
		
		// 조건이 바뀌었으므로 Behavior Tree에게 "나 다시 체크해줘(=Abort 등 처리해줘)" 라고 요청
		OwnerComp.RequestExecution(this);
	}
}

// [삭제됨] OnCeaseRelevant
// 공격이 끝나거나 트리가 재평가될 때 슬롯을 반납해버리는 문제를 막기 위해 함수 자체를 삭제했습니다.