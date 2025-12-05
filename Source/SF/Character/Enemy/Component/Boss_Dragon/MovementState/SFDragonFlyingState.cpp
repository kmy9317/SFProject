// SFDragonFlyingState.cpp
#include "SFDragonFlyingState.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/Enemy/Component/Boss_Dragon/DragonMovementComponent.h"

USFDragonFlyingState* USFDragonFlyingState::CreateState(UObject* Outer, USFDragonMovementComponent* OwnerComponent)
{
	USFDragonFlyingState* NewState = NewObject<USFDragonFlyingState>(Outer);
	if (NewState && OwnerComponent)
	{
		NewState->Initialize(OwnerComponent);
	}
	return NewState;
}

void USFDragonFlyingState::EnterState()
{
    Super::EnterState();
    
    if (OwnerMovementComponent && CharacterMovement)
    {
        CharacterMovement->SetMovementMode(MOVE_None);
        CharacterMovement->GravityScale = 0.0f;
        CharacterMovement->StopMovementImmediately();
        
    }
}

void USFDragonFlyingState::UpdateState(float DeltaTime)
{
    Super::UpdateState(DeltaTime);

    if (OwnerMovementComponent)
    {
        UpdateFlying(DeltaTime);
    }

}

void USFDragonFlyingState::ExitState()
{
    
    
    Super::ExitState();
}

FGameplayTag USFDragonFlyingState::GetType() const
{
    return SFGameplayTags::Dragon_Movement_Flying;
}

void USFDragonFlyingState::UpdateFlying(float DeltaTime)
{
    if (!OwnerMovementComponent) return;

    AActor* OwnerActor = OwnerMovementComponent->GetOwner();
    AActor* Target = OwnerMovementComponent->GetTargetActor();
    if (!OwnerActor || !Target) return;

    FVector CurrentLocation = OwnerActor->GetActorLocation();
    FVector TargetLocation = Target->GetActorLocation() + OwnerMovementComponent->GetTargetOffset();

    float Speed = OwnerMovementComponent->GetCurrentFlightSpeed();
    FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();

    FVector NewLocation = CurrentLocation + Direction * Speed * DeltaTime;
    OwnerActor->SetActorLocation(NewLocation, true);

    // 회전
    FRotator CurrentRotation = OwnerActor->GetActorRotation();
    FRotator TargetRotation = Direction.Rotation();
    float RotSpeed = OwnerMovementComponent->GetFlightRotationSpeed();

    FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotSpeed);
    OwnerActor->SetActorRotation(NewRotation);
}
