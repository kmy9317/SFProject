#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlueprintBase.h"
#include "BTD_GuardSlotAvailable.generated.h"

// NodeMemory: 이전 슬롯 상태 저장
struct FSFGuardSlotMemory
{
	bool bLastResult = false;
	bool bInitialized = false; // 초기화 플래그
};

/**
 * SFBTD_GuardSlotAvailable
 * (구 BTD_MeleeSlotAvailable)
 *
 * 공격/가드 슬롯이 사용 가능한지 체크하는 데코레이터
 * - SFEnemyController의 bHasGuardSlot 상태를 확인
 * - CombatSlotManager를 통해 슬롯 요청/해제
 *
 * Observer Aborts:
 * - 실시간으로 슬롯 상태 체크 (Tick)하여 상태 변경 시 트리를 중단/재시작
 */
UCLASS()
class SF_API UBtd_GuardSlotAvailable : public UBTDecorator_BlueprintBase
{
	GENERATED_BODY()

public:
	UBtd_GuardSlotAvailable();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FSFGuardSlotMemory); }
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};