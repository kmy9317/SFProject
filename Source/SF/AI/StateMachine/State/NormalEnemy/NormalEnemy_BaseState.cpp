// Fill out your copyright notice in the Description page of Project Settings.


#include "NormalEnemy_BaseState.h"

#include "AI/StateMachine/SFStateMachine.h"

void UNormalEnemy_BaseState::OnEnter_Implementation()
{
	Super::OnEnter_Implementation();

	if (StateMachine)
	{
		if (StateMachine->OnChangeTreeDelegate.IsBound())
		{
			StateMachine->OnChangeTreeDelegate.Broadcast(BehaviourTag);
		}
	}
	
}
