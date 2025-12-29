#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTD_CheckHealthRatio.generated.h"

/**
 * USFPrimarySet의 Health/MaxHealth를 확인하여 비율을 검사하는 데코레이터
 * 예: Threshold가 0.5라면 체력이 50% 이하일 때 True 반환
 */
UCLASS()
class SF_API UBTD_CheckHealthRatio : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTD_CheckHealthRatio();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;

	// 체력 비율 임계값 (0.0 ~ 1.0)
	// 예: 0.5 = 50%
	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HealthThreshold = 0.5f;
};