// Fill out your copyright notice in the Description page of Project Settings.

#include "SFBTTask_SetRotationMode.h"
#include "AIController.h"
#include "AI/Controller/SFBaseAIController.h"

USFBTTask_SetRotationMode::USFBTTask_SetRotationMode()
{
	NodeName = "SF Set Rotation Mode";
	bNotifyTaskFinished = false;
}

EBTNodeResult::Type USFBTTask_SetRotationMode::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC)
	{
		return EBTNodeResult::Failed;
	}

	ASFBaseAIController* SFController = Cast<ASFBaseAIController>(AIC);
	if (!SFController)
	{
		return EBTNodeResult::Failed;
	}

	// 회전 모드 설정
	SFController->SetRotationMode(RotationMode);

	return EBTNodeResult::Succeeded;
}

