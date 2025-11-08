// Fill out your copyright notice in the Description page of Project Settings.


#include "SFEnemyData.h"

UBehaviorTree* FSFBehaviourWrapperContainer::GetBehaviourTree(const FGameplayTag& Tag) const
{
	for (auto BehaviourWrapper : Behaviours)
	{
		if (Tag.MatchesTagExact(BehaviourWrapper.BehaviourTag))
		{
			return BehaviourWrapper.Behaviour;
		}
	}
	return nullptr;
}
