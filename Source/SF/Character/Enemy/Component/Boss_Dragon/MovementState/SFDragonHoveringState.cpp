// Fill out your copyright notice in the Description page of Project Settings.


#include "SFDragonHoveringState.h"

#include "Character/Enemy/Component/Boss_Dragon/DragonMovementComponent.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"

USFDragonHoveringState* USFDragonHoveringState::CreateState(UObject* Outer, USFDragonMovementComponent* OwnerComponent)
{
	USFDragonHoveringState* NewState = NewObject<USFDragonHoveringState>(Outer);
	if (NewState && OwnerComponent)
	{
		NewState->Initialize(OwnerComponent);
	}
	return NewState;
}

void USFDragonHoveringState::EnterState()
{
	Super::EnterState();
	
	if (OwnerMovementComponent && OwnerMovementComponent->GetCharacterMovementComponent())
	{
		PivotLocation = OwnerMovementComponent->GetOwner()->GetActorLocation();
		OwnerMovementComponent->GetCharacterMovementComponent()->SetMovementMode(MOVE_None);
		OwnerMovementComponent->GetCharacterMovementComponent()->GravityScale = 0.0f;
		OwnerMovementComponent->GetCharacterMovementComponent()->StopMovementImmediately();
		HoveringLocation = PivotLocation + HoveringDirection * OwnerMovementComponent->GetHoveringDistance();		
	}
}

void USFDragonHoveringState::UpdateState(float DeltaTime)
{
	Super::UpdateState(DeltaTime);

	UpdateHovering(DeltaTime);
}

void USFDragonHoveringState::ExitState()
{
	HoveringDirection  = FVector(0.0f,0.0f,1.0f);
	Super::ExitState();
}

FGameplayTag USFDragonHoveringState::GetType() const
{
	return SFGameplayTags::Dragon_Movement_Hovering;
}

void USFDragonHoveringState::UpdateHovering(float DeltaTime)
{
	if (!OwnerMovementComponent)
		return;

	AActor* Owner = OwnerMovementComponent->GetOwner();
	FVector ActorLoc = Owner->GetActorLocation();


	float Dist = FVector::Dist(ActorLoc, HoveringLocation);
	if (Dist < 2.f) 
	{
		Owner->SetActorLocation(HoveringLocation, true);
		// 방향 반전
		HoveringDirection *= -1;
		
		HoveringLocation = PivotLocation + HoveringDirection * OwnerMovementComponent->GetHoveringDistance();
	}


	FVector NewLoc = FMath::VInterpConstantTo(
		ActorLoc,
		HoveringLocation,
		DeltaTime,
		OwnerMovementComponent->GetHoveringSpeed()
	);

	Owner->SetActorLocation(NewLoc, true);
}

