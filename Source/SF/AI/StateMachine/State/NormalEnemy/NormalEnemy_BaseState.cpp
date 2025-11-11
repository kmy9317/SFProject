// Fill out your copyright notice in the Description page of Project Settings.


#include "NormalEnemy_BaseState.h"
#include "AI/StateMachine/SFStateMachine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NormalEnemy_BaseState)

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
