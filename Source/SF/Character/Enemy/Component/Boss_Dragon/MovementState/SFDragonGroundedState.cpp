// Fill out your copyright notice in the Description page of Project Settings.


#include "SFDragonGroundedState.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"

USFDragonGroundedState* USFDragonGroundedState::CreateState(UObject* Outer, USFDragonMovementComponent* OwnerComponent)
{
	USFDragonGroundedState* NewState = NewObject<USFDragonGroundedState>(Outer);
	if (NewState && OwnerComponent)
	{
		NewState->Initialize(OwnerComponent);
	}
	return NewState;
}

void USFDragonGroundedState::EnterState()
{
	Super::EnterState();
	if (OwnerMovementComponent && CharacterMovement)
	{
		CharacterMovement->GravityScale = 1.0f;
		CharacterMovement->SetMovementMode(MOVE_Walking);
	}
}

void USFDragonGroundedState::UpdateState(float DeltaTime)
{
	Super::UpdateState(DeltaTime);
	
}

void USFDragonGroundedState::ExitState()
{
	if (CharacterMovement)
	{
		CharacterMovement->SetMovementMode(MOVE_None);
	}
	Super::ExitState();
}

FGameplayTag USFDragonGroundedState::GetType() const
{
	return SFGameplayTags::Dragon_Movement_Grounded;
}
