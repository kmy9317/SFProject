// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlueprintBase.h"
#include "BTD_GuardSlotAvailable.generated.h"

// NodeMemory: 이전 프레임의 슬롯 보유 상태 저장 (변경 감지용)
struct FSFGuardSlotMemory
{
	bool bLastResult = false;
	bool bInitialized = false; 
};

/**
 * SFBTD_GuardSlotAvailable
 * (구 BTD_MeleeSlotAvailable)
 *
 * [리팩토링 수정 사항]
 * - 기존: 슬롯 요청(Request) 및 해제(Release)를 직접 수행 -> 버그 원인
 * - 변경: 오직 SFEnemyController의 bHasGuardSlot 변수만 '확인'하는 문지기 역할
 * - 슬롯의 실제 확보와 유지는 상위 Service(BTService_UpdateTarget)에서 전담함
 */
UCLASS()
class SF_API UBtd_GuardSlotAvailable : public UBTDecorator_BlueprintBase
{
	GENERATED_BODY()

public:
	UBtd_GuardSlotAvailable();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	
	// 실시간 상태 감지를 위해 Tick 사용
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FSFGuardSlotMemory); }
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	
	// [삭제] OnCeaseRelevant 제거됨 (슬롯 해제 로직 삭제)
	// virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};