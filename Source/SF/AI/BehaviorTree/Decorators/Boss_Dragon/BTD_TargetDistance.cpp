// Fill out your copyright notice in the Description page of Project Settings.


#include "BTD_TargetDistance.h"

#include "BehaviorTree/BlackboardComponent.h"

UBTD_TargetDistance::UBTD_TargetDistance()
{
	DistanceKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTD_TargetDistance, DistanceKey));
	NodeName = "Target Distance";
}

bool UBTD_TargetDistance::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	float Distance = 0.0f;
	if (DistanceKey.IsSet())
	{
		Distance = OwnerComp.GetBlackboardComponent()->GetValueAsFloat(DistanceKey.SelectedKeyName);
	}
	switch (Operator)
	{
	case EArithmeticKeyOperation::Equal:
		return Distance == DistanceThreshold;
		break;
	case EArithmeticKeyOperation::NotEqual:
		return Distance != DistanceThreshold;
		break;
	case EArithmeticKeyOperation::Less:
		return Distance < DistanceThreshold;
		break;
	case EArithmeticKeyOperation::LessOrEqual:
		return Distance <= DistanceThreshold;
		break;
	case EArithmeticKeyOperation::Greater:
		return Distance > DistanceThreshold;
		break;
	case EArithmeticKeyOperation::GreaterOrEqual:
		return Distance >= DistanceThreshold;
		break;
	default:
		return false;
		break;
	}
}
