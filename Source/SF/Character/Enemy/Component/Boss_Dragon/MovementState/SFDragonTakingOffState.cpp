// Fill out your copyright notice in the Description page of Project Settings.


#include "SFDragonTakingOffState.h"

#include "Character/Enemy/Component/Boss_Dragon/DragonMovementComponent.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"

USFDragonTakingOffState* USFDragonTakingOffState::CreateState(UObject* Outer, USFDragonMovementComponent* OwnerComponent)
{
	USFDragonTakingOffState* NewState = NewObject<USFDragonTakingOffState>(Outer);
	if (NewState && OwnerComponent)
	{
		NewState->Initialize(OwnerComponent);
	}
	return NewState;
}

void USFDragonTakingOffState::EnterState()
{
	Super::EnterState();
	if (OwnerMovementComponent && OwnerMovementComponent->GetCharacterMovementComponent())
	{
		OwnerMovementComponent->GetCharacterMovementComponent()->SetMovementMode(MOVE_None);
		OwnerMovementComponent->GetCharacterMovementComponent()->GravityScale = 0.f; // 중력 끄기
		
	}
}

void USFDragonTakingOffState::UpdateState(float DeltaTime)
{
	Super::UpdateState(DeltaTime);

	if (OwnerMovementComponent)
	{
		UpdateTakeOff(DeltaTime);
	}
}

void USFDragonTakingOffState::ExitState()
{
	Super::ExitState();
}

FGameplayTag USFDragonTakingOffState::GetType() const
{
	return SFGameplayTags::Dragon_Movement_TakingOff;
}

void USFDragonTakingOffState::UpdateTakeOff(float DeltaTime)
{
	if (!OwnerMovementComponent) return;

	AActor* Owner = OwnerMovementComponent->GetOwner();
	if (!Owner) return;

	const float TargetHeight = OwnerMovementComponent->GetMinimumFlightHeight();
	FVector CurrentLoc = Owner->GetActorLocation();


	float TakeOffSpeed = OwnerMovementComponent->GetTakeOffSpeed(); 
	float Direction = FMath::Sign(TargetHeight - CurrentLoc.Z);  
	float DeltaZ = Direction * TakeOffSpeed * DeltaTime;

	float NewZ = CurrentLoc.Z + DeltaZ;

	
	if ((Direction > 0 && NewZ > TargetHeight) || (Direction < 0 && NewZ < TargetHeight))
	{
		NewZ = TargetHeight;
	}

	FVector NewLocation = CurrentLoc;
	NewLocation.Z = NewZ;
	Owner->SetActorLocation(NewLocation, true);

	// 부드럽게 위쪽으로 회전
	FRotator CurrentRot = Owner->GetActorRotation();
	FRotator TargetRot(-20.f, CurrentRot.Yaw, CurrentRot.Roll);
	FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 2.0f);
	Owner->SetActorRotation(NewRot);

	// 목표 고도 도달 시 Flying State로 전환
	if (FMath::IsNearlyEqual(NewZ, TargetHeight, 1.f))
	{
		OwnerMovementComponent->SetMovementState(SFGameplayTags::Dragon_Movement_Flying);
	}
}

