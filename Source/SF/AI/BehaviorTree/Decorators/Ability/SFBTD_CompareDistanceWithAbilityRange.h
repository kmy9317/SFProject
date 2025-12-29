// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BehaviorTree/Blackboard/BlackboardKeyEnums.h"
#include "SFBTD_CompareDistanceWithAbilityRange.generated.h"



USTRUCT()
struct FBTDistanceCompareMemory
{
	GENERATED_BODY()

	bool bLastResult = false;
};

/**
 * BTDecorator: Target과의 거리와 Ability Range 비교
 */
UCLASS()
class SF_API USFBTD_CompareDistanceWithAbilityRange : public UBTDecorator
{
	GENERATED_BODY()

public:
	USFBTD_CompareDistanceWithAbilityRange();

	
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetKey;

	
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector DistanceKey;

	
	UPROPERTY(EditAnywhere, Category = "Ability")
	FBlackboardKeySelector AbilityTagKey;

	
	UPROPERTY(EditAnywhere, Category = "Decorator")
	TEnumAsByte<EArithmeticKeyOperation::Type> Operator;

protected:
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	float GetAbilityAttackRange(UBehaviorTreeComponent& OwnerComp, const FGameplayTag& AbilityTag) const;
	float CalculateDistance(UBehaviorTreeComponent& OwnerComp) const;
};

