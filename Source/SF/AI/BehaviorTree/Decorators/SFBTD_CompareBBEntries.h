// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/Blackboard/BlackboardKeyEnums.h"
#include "SFBTD_CompareBBEntries.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFBTD_CompareBBEntries : public UBTDecorator
{
	GENERATED_BODY()

public:
	USFBTD_CompareBBEntries();

protected:
	
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	
	UPROPERTY(EditAnywhere, Category = "Condition")
	FBlackboardKeySelector LeftKey;
	
	UPROPERTY(EditAnywhere, Category = "Condition")
	TEnumAsByte<EArithmeticKeyOperation::Type> Operator;
	
	UPROPERTY(EditAnywhere, Category = "Condition")
	FBlackboardKeySelector RightKey;
};
