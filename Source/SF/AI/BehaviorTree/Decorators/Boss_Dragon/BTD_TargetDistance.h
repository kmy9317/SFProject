// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/Blackboard/BlackboardKeyEnums.h"
#include "BTD_TargetDistance.generated.h"

/**
 * 
 */
UCLASS()
class SF_API UBTD_TargetDistance : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTD_TargetDistance();
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
public:
	UPROPERTY(EditAnywhere, Category= "Blackboard")
	FBlackboardKeySelector DistanceKey;

	UPROPERTY(EditAnywhere,Category= "float")
	float DistanceThreshold = 1500.f;

	UPROPERTY(EditAnywhere, Category= "Operator")
	TEnumAsByte<EArithmeticKeyOperation::Type> Operator;
};
