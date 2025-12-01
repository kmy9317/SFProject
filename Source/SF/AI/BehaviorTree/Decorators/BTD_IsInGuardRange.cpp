// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BehaviorTree/Decorators/BTD_IsInGuardRange.h"

#include "AI/Controller/SFEnemyController.h"

UBTD_IsInGuardRange::UBTD_IsInGuardRange()
{
	NodeName = "Is In Guard Range";

	// Observer Aborts본값
	FlowAbortMode = EBTFlowAbortMode::LowerPriority;

	//Tick 활성화
	bNotifyTick = true;
}

bool UBTD_IsInGuardRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (ASFEnemyController* AI = Cast<ASFEnemyController>(OwnerComp.GetAIOwner()))
	{
		return AI->IsInGuardRange();
	}
	return false;	
}

void UBTD_IsInGuardRange::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FBTGuardRangeMemory* Memory = reinterpret_cast<FBTGuardRangeMemory*>(NodeMemory);
	if (!Memory)
	{
		return;
	}

	// 현재 조건 값 계산
	const bool bCurrentResult = CalculateRawConditionValue(OwnerComp, NodeMemory);

	//값이 바뀌었을 때만 재평가 요청!
	if (Memory->bLastResult != bCurrentResult)
	{

		Memory->bLastResult = bCurrentResult;
		OwnerComp.RequestExecution(this);
	}
}