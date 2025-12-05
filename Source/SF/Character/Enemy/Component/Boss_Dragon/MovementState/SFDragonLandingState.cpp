// Fill out your copyright notice in the Description page of Project Settings.


#include "SFDragonLandingState.h"

#include "Character/Enemy/Component/Boss_Dragon/DragonMovementComponent.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"

USFDragonLandingState* USFDragonLandingState::CreateState(UObject* Outer, USFDragonMovementComponent* OwnerComponent)
{
	USFDragonLandingState* NewState = NewObject<USFDragonLandingState>(Outer);
	if (NewState && OwnerComponent)
	{
		NewState->Initialize(OwnerComponent);
	}
	return NewState;
}

void USFDragonLandingState::EnterState()
{
	Super::EnterState();
	if (OwnerMovementComponent && OwnerMovementComponent->GetCharacterMovementComponent())
	{
		OwnerMovementComponent->GetCharacterMovementComponent()->SetMovementMode(MOVE_Walking);
		OwnerMovementComponent->GetCharacterMovementComponent()->GravityScale = 1.0f;
	}
}

void USFDragonLandingState::UpdateState(float DeltaTime)
{
	Super::UpdateState(DeltaTime);

	if (OwnerMovementComponent)
	{
		UpdateLanding(DeltaTime);
	}
}

void USFDragonLandingState::ExitState()
{
	Super::ExitState();
}

FGameplayTag USFDragonLandingState::GetType() const
{
	return SFGameplayTags::Dragon_Movement_Landing;
}

void USFDragonLandingState::UpdateLanding(float DeltaTime)
{
	if (!OwnerMovementComponent) return;

	AActor* Owner = OwnerMovementComponent->GetOwner();
	if (!Owner) return;

	FVector CurrentLoc = Owner->GetActorLocation();
	FVector TargetLoc = OwnerMovementComponent->GetTargetLandingLocation();

	// 착지 위치로 부드럽게 이동
	FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLoc, DeltaTime, OwnerMovementComponent->GetLandingSpeed() / 100.0f);
	Owner->SetActorLocation(NewLoc, true);

	// 회전 
	FRotator CurrentRot = Owner->GetActorRotation();
	FRotator TargetRot = FRotator(0.f, CurrentRot.Yaw, 0.f);
	FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 2.0f);
	Owner->SetActorRotation(NewRot);

	// 지면 도달 체크
	float GroundDist = OwnerMovementComponent->GetGroundDistance();

	// 지면에 거의 도달했거나 목표 위치에 도달한 경우
	if (GroundDist < 10.f || FVector::Dist(NewLoc, TargetLoc) < 50.f)
	{
		
		OwnerMovementComponent->SetMovementState(SFGameplayTags::Dragon_Movement_Grounded);
	}
}
