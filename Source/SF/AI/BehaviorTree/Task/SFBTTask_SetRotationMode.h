// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "AI/Controller/SFBaseAIController.h"
#include "SFBTTask_SetRotationMode.generated.h"

/**
 * BehaviorTree 태스크: AI Controller의 회전 모드를 설정합니다.
 * 
 * 사용 예시:
 * - 공격 준비 시 TurnInPlace 모드로 전환
 * - 순찰 시 MovementDirection 모드로 전환
 * - 전투 시 ControllerYaw 모드로 전환
 */
UCLASS()
class SF_API USFBTTask_SetRotationMode : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USFBTTask_SetRotationMode();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	// 설정할 회전 모드
	UPROPERTY(EditAnywhere, Category = "AI|Rotation")
	EAIRotationMode RotationMode = EAIRotationMode::ControllerYaw;
};

