// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlueprintBase.h"
#include "BTD_CheckSpecialAttack.generated.h"

/**
 * 
 */
UCLASS()
class SF_API UBTD_CheckSpecialAttack : public UBTDecorator_BlueprintBase
{
	GENERATED_BODY()
	
public:
	UBTD_CheckSpecialAttack();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
protected:
	// 확률 20%
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Probability = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AbilityTagName;
		
	
	
	
};
