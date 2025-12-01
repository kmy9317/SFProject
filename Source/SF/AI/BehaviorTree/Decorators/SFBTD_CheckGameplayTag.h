// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlueprintBase.h"
#include "GameplayTagContainer.h"
#include "SFBTD_CheckGameplayTag.generated.h"

/**
 * AI(Pawn)의 ASC에 특정 GameplayTag가 있는지 확인하는 범용 데코레이터
 * - 기존의 IsInMeleeRange, IsInGuardRange, IsInTrackingRange 등을 모두 대체함
 * - 태그 상태가 변하면 즉시 Behavior Tree의 흐름을 제어함
 */
UCLASS()
class SF_API USFBTD_CheckGameplayTag : public UBTDecorator_BlueprintBase
{
	GENERATED_BODY()

public:
	USFBTD_CheckGameplayTag();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    
	// 단순 bool 값 하나만 저장하면 되므로 메모리 크기 최소화
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(bool); }

protected:
	// 에디터에서 설정할 태그 (예: AI.Range.Melee)
	UPROPERTY(EditAnywhere, Category = "Condition")
	FGameplayTag TagToCheck;
	
	// 태그가 '없을 때' 성공으로 칠 것인가? (NOT 조건)
	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bInvertCondition = false;
};