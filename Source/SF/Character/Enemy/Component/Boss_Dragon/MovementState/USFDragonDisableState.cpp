// Fill out your copyright notice in the Description page of Project Settings.


#include "USFDragonDisableState.h"

#include "Character/Enemy/Component/Boss_Dragon/DragonMovementComponent.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"

USFDragonDisabledState* USFDragonDisabledState::CreateState(UObject* Outer, USFDragonMovementComponent* OwnerComponent)
{
	USFDragonDisabledState* NewState = NewObject<USFDragonDisabledState>(Outer);
	if (NewState && OwnerComponent)
	{
		NewState->Initialize(OwnerComponent);
	}
	return NewState;
}

void USFDragonDisabledState::EnterState()
{
	Super::EnterState();
	if (OwnerMovementComponent && OwnerMovementComponent->GetCharacterMovementComponent())
	{
		OwnerMovementComponent->GetCharacterMovementComponent()->SetMovementMode(MOVE_None);
		OwnerMovementComponent->GetCharacterMovementComponent()->GravityScale = 0.0f;
		OwnerMovementComponent->GetCharacterMovementComponent()->StopMovementImmediately();
	}
}

void USFDragonDisabledState::UpdateState(float DeltaTime)
{
	Super::UpdateState(DeltaTime);
	// Disabled 상태에서는 아무것도 하지 않음
}

void USFDragonDisabledState::ExitState()
{
	if (OwnerMovementComponent && OwnerMovementComponent->GetCharacterMovementComponent())
	{
		OwnerMovementComponent->GetCharacterMovementComponent()->SetMovementMode(MOVE_Walking);
		OwnerMovementComponent->GetCharacterMovementComponent()->GravityScale = 1.0f;
	}
	Super::ExitState();
}

FGameplayTag USFDragonDisabledState::GetType() const
{
	return SFGameplayTags::Dragon_Movement_Disabled;
}
