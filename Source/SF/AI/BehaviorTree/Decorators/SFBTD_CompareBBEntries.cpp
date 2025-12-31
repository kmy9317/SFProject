// Fill out your copyright notice in the Description page of Project Settings.


#include "SFBTD_CompareBBEntries.h"

#include "BehaviorTree/BlackboardComponent.h"

USFBTD_CompareBBEntries::USFBTD_CompareBBEntries()
{
	NodeName = "Compare Blackboard entries";
	LeftKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(USFBTD_CompareBBEntries, LeftKey));
	RightKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(USFBTD_CompareBBEntries, RightKey));
	
}

bool USFBTD_CompareBBEntries::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return false;

	
	float LeftValue = BB->GetValueAsFloat(LeftKey.SelectedKeyName);
	float RightValue = BB->GetValueAsFloat(RightKey.SelectedKeyName);
	
	switch (Operator)
	{
	case EArithmeticKeyOperation::Equal:          return FMath::IsNearlyEqual(LeftValue, RightValue);
	case EArithmeticKeyOperation::NotEqual:       return !FMath::IsNearlyEqual(LeftValue, RightValue);
	case EArithmeticKeyOperation::Less:            return LeftValue < RightValue;
	case EArithmeticKeyOperation::LessOrEqual:       return LeftValue <= RightValue;
	case EArithmeticKeyOperation::Greater:         return LeftValue > RightValue;
	case EArithmeticKeyOperation::GreaterOrEqual:    return LeftValue >= RightValue;
	default: return false;
	}
}