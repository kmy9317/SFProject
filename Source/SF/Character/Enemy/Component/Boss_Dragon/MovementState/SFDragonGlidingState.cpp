// Fill out your copyright notice in the Description page of Project Settings.


#include "SFDragonGlidingState.h"

#include "Character/Enemy/Component/Boss_Dragon/DragonMovementComponent.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"

USFDragonGlidingState* USFDragonGlidingState::CreateState(UObject* Outer, USFDragonMovementComponent* OwnerComponent)
{
	USFDragonGlidingState* NewState = NewObject<USFDragonGlidingState>(Outer);
	if (NewState && OwnerComponent)
	{
		NewState->Initialize(OwnerComponent);
	}
	return NewState;
}

void USFDragonGlidingState::EnterState()
{
	Super::EnterState();
	if (OwnerMovementComponent && OwnerMovementComponent->GetCharacterMovementComponent())
	{
		OwnerMovementComponent->GetCharacterMovementComponent()->SetMovementMode(MOVE_None);
		OwnerMovementComponent->GetCharacterMovementComponent()->GravityScale = 0.0f;
		OwnerMovementComponent->GetCharacterMovementComponent()->StopMovementImmediately();
	}
	
}

void USFDragonGlidingState::UpdateState(float DeltaTime)
{
	Super::UpdateState(DeltaTime);
	UpdateGliding(DeltaTime);
}

void USFDragonGlidingState::ExitState()
{
	Super::ExitState();
}

FGameplayTag USFDragonGlidingState::GetType() const
{
	return SFGameplayTags::Dragon_Movement_Gliding;
}

void USFDragonGlidingState::UpdateGliding(float DeltaTime)
{
	if (!OwnerMovementComponent) return;

	AActor* Owner = OwnerMovementComponent->GetOwner();
	if (!Owner) return;

	
	FVector TargetLoc = OwnerMovementComponent->GetTargetLandingLocation();
	FVector CurrentLoc = Owner->GetActorLocation();

	
	FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLoc, DeltaTime, OwnerMovementComponent->GetLandingSpeed() / 100.0f);
	Owner->SetActorLocation(NewLoc, true);

	// 회전
	FRotator CurrentRot = Owner->GetActorRotation();
	FRotator TargetRot = FRotator(-20.f, CurrentRot.Yaw, CurrentRot.Roll);
	FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 2.0f);
	Owner->SetActorRotation(NewRot);

	// 지면 근처 도달 시 Landing State
	float GroundDist = OwnerMovementComponent->GetGroundDistance();
	if (GroundDist < 100.f)
	{
		UE_LOG(LogTemp, Log, TEXT("Gliding near ground! Transitioning to Landing State"));
		OwnerMovementComponent->SetMovementState(SFGameplayTags::Dragon_Movement_Landing);
	}
}
