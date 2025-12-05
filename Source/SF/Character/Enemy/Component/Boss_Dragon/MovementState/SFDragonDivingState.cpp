// Fill out your copyright notice in the Description page of Project Settings.


#include "SFDragonDivingState.h"

#include "Character/Enemy/Component/Boss_Dragon/DragonMovementComponent.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"

USFDragonDivingState* USFDragonDivingState::CreateState(UObject* Outer, USFDragonMovementComponent* OwnerComponent)
{
	USFDragonDivingState* NewState = NewObject<USFDragonDivingState>(Outer);
	if (NewState && OwnerComponent)
	{
		NewState->Initialize(OwnerComponent);
	}
	return NewState;
}

void USFDragonDivingState::EnterState()
{
	Super::EnterState();

	if (!OwnerMovementComponent) return;

	auto* Move = OwnerMovementComponent->GetCharacterMovementComponent();
	if (Move)
	{
		Move->SetMovementMode(MOVE_None);
		Move->GravityScale = 0.f;
		Move->StopMovementImmediately();
	}

	// Dive 시작 순간 목표 지점 고정
	DiveTarget = OwnerMovementComponent->GetTargetActor();
}

void USFDragonDivingState::UpdateState(float DeltaTime)
{
	Super::UpdateState(DeltaTime);
	UpdateDiving(DeltaTime);
}

void USFDragonDivingState::ExitState()
{
	Super::ExitState();
}

void USFDragonDivingState::UpdateDiving(float DeltaTime)
{
	if (!OwnerMovementComponent) return;

	AActor* Owner = OwnerMovementComponent->GetOwner();
	if (!Owner) return;

	// DiveTarget null 체크
	if (!DiveTarget)
	{
		OwnerMovementComponent->SetMovementState(SFGameplayTags::Dragon_Movement_Hovering);
		return;
	}

	FVector ActorLoc = Owner->GetActorLocation();
	FVector TargetLoc = DiveTarget->GetActorLocation();

	
	FVector ToTargetDir = (TargetLoc - ActorLoc).GetSafeNormal();

	float DiveSpeed   = OwnerMovementComponent->GetDiveSpeed();        
	float DiveGravity = OwnerMovementComponent->GetDiveGravity();  

	// 이동 
	FVector Velocity = ToTargetDir * DiveSpeed + FVector(0.f, 0.f, -1.f) * DiveGravity;
	FVector NewLoc = ActorLoc + Velocity * DeltaTime;

	Owner->SetActorLocation(NewLoc, true);

	// 회전 
	FRotator TargetRot = ToTargetDir.Rotation();
	FRotator NewRot = FMath::RInterpTo(Owner->GetActorRotation(), TargetRot, DeltaTime, OwnerMovementComponent->GetDivingRotationSpeed());
	Owner->SetActorRotation(NewRot);

	// 도착 판정
	float Dist = FVector::Dist(NewLoc, TargetLoc);
	float GroundDist = OwnerMovementComponent->GetGroundDistance();

	// 목표 지점 근처 도달 
	if (Dist < 150.f || GroundDist < 200.f)
	{
		OwnerMovementComponent->SetMovementState(SFGameplayTags::Dragon_Movement_Hovering);
	}
}

FGameplayTag USFDragonDivingState::GetType() const
{
	return SFGameplayTags::Dragon_Movement_Diving;
}
