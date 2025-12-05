// Fill out your copyright notice in the Description page of Project Settings.


#include "SFDragonMovementStateBase.h"

#include "Character/Enemy/Component/Boss_Dragon/DragonMovementComponent.h"
#include "GameFramework/Character.h"

void USFDragonMovementStateBase::Initialize(class USFDragonMovementComponent* InOwnerMovement)
{
	OwnerMovementComponent = InOwnerMovement;
	OwnerCharacter = Cast<ACharacter>(InOwnerMovement->GetOwner());
	if (OwnerCharacter)
	{
		CharacterMovement = OwnerCharacter->GetCharacterMovement();
	}
	
}

bool USFDragonMovementStateBase::CanTransitionTo()
{
	return true;
}
