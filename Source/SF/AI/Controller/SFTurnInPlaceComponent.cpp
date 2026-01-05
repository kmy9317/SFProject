#include "SFTurnInPlaceComponent.h"
#include "SFBaseAIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFTurnInPlaceComponent)

USFTurnInPlaceComponent::USFTurnInPlaceComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetComponentTickEnabled(false);

    bIsTurning = false;
    LockedDeltaYaw = 0.f;
    TurnThreshold = 45.f;
    LargeTurnThreshold = 135.f;
}

void USFTurnInPlaceComponent::InitializeTurnInPlaceComponent()
{
    bIsTurning = false;
    LockedDeltaYaw = 0.f;
}

ASFBaseAIController* USFTurnInPlaceComponent::GetAIController() const
{
    return Cast<ASFBaseAIController>(GetOwner());
}

APawn* USFTurnInPlaceComponent::GetControlledPawn() const
{
    if (AController* Controller = Cast<AController>(GetOwner()))
    {
        return Controller->GetPawn();
    }
    return nullptr;
}

UAbilitySystemComponent* USFTurnInPlaceComponent::GetASC() const
{
    if (APawn* Pawn = GetControlledPawn())
    {
        return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
    }
    return nullptr;
}

AActor* USFTurnInPlaceComponent::GetTargetActor() const
{
    if (ASFBaseAIController* AI = GetAIController())
    {
        return AI->TargetActor;
    }
    return nullptr;
}

float USFTurnInPlaceComponent::GetAngleToTarget() const
{
    APawn* Pawn = GetControlledPawn();
    AActor* Target = GetTargetActor();
    if (!Pawn || !Target) return 0.f;

    FVector ToTarget = Target->GetActorLocation() - Pawn->GetActorLocation();
    ToTarget.Z = 0.f;
    if (ToTarget.IsNearlyZero()) return 0.f;

    const FRotator TargetRot = ToTarget.Rotation();
    return FMath::FindDeltaAngleDegrees(Pawn->GetActorRotation().Yaw, TargetRot.Yaw);
}

void USFTurnInPlaceComponent::SyncControlRotationToTarget()
{
    ASFBaseAIController* AI = GetAIController();
    APawn* Pawn = GetControlledPawn();
    AActor* Target = GetTargetActor();
    if (!AI || !Pawn || !Target) return;

    FVector ToTarget = Target->GetActorLocation() - Pawn->GetActorLocation();
    ToTarget.Z = 0.f;
    if (!ToTarget.IsNearlyZero())
    {
        AI->SetControlRotation(ToTarget.Rotation());
    }
}

void USFTurnInPlaceComponent::RequestTurnToTarget(AActor* Target)
{
    if (!Target || bIsTurning) return;

    APawn* Pawn = GetControlledPawn();
    ASFBaseAIController* AI = GetAIController();
    if (!Pawn || !AI) return;

    FVector ToTarget = Target->GetActorLocation() - Pawn->GetActorLocation();
    ToTarget.Z = 0.f;
    if (ToTarget.IsNearlyZero()) return;

    const FRotator TargetRot = ToTarget.Rotation();
    const float YawDelta = FMath::FindDeltaAngleDegrees(Pawn->GetActorRotation().Yaw, TargetRot.Yaw);
    const float AbsYaw = FMath::Abs(YawDelta);

    if (AbsYaw >= TurnThreshold)
    {
        
        DisableSmoothRotation();
        AI->SetRotationMode(EAIRotationMode::None);

        if (ACharacter* Char = Cast<ACharacter>(Pawn))
        {
            Char->bUseControllerRotationYaw = false;
        }
        AI->SetControlRotation(TargetRot);
        ExecuteTurn(YawDelta);
        return;
    }
    if (AbsYaw > AcceptableAngle) 
    {
        FRotator NewRot = FMath::RInterpTo(Pawn->GetActorRotation(), TargetRot, GetWorld()->GetDeltaSeconds(), RotationInterpSpeed);
        Pawn->SetActorRotation(NewRot);
        AI->SetControlRotation(NewRot);
    }
}

void USFTurnInPlaceComponent::ExecuteTurn(float DeltaYaw)
{
    ASFBaseAIController* AI = GetAIController();
    APawn* Pawn = GetControlledPawn();
    if (!AI || !Pawn) return;

    DisableSmoothRotation();

    bIsTurning = true;
    LockedDeltaYaw = DeltaYaw;

    const float AbsDeltaYaw = FMath::Abs(LockedDeltaYaw);
    FGameplayTag EventTag;

    if (AbsDeltaYaw >= LargeTurnThreshold)
    {
        EventTag = (LockedDeltaYaw > 0.f) ? SFGameplayTags::GameplayEvent_Turn_180R : SFGameplayTags::GameplayEvent_Turn_180L;
    }
    else
    {
        EventTag = (LockedDeltaYaw > 0.f) ? SFGameplayTags::GameplayEvent_Turn_90R : SFGameplayTags::GameplayEvent_Turn_90L;
    }

    FGameplayEventData Payload;
    Payload.EventTag = EventTag;
    Payload.Instigator = Pawn;
    Payload.EventMagnitude = LockedDeltaYaw;

    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn, EventTag, Payload);
}

void USFTurnInPlaceComponent::OnTurnFinished()
{
    APawn* Pawn = GetControlledPawn();
    ASFBaseAIController* AI = GetAIController();
    if (!AI || !Pawn) return;

    bIsTurning = false;
    LockedDeltaYaw = 0.f;
    
    AI->SetControlRotation(Pawn->GetActorRotation());
    
    if (AI->TargetActor)
    {
        AI->SetRotationMode(EAIRotationMode::ControllerYaw);
    }
    else
    {
        AI->SetRotationMode(EAIRotationMode::MovementDirection);
    }

    if (ACharacter* Char = Cast<ACharacter>(Pawn))
    {
        Char->bUseControllerRotationYaw = false;
    }
}

void USFTurnInPlaceComponent::DisableSmoothRotation()
{
    if (ACharacter* Char = Cast<ACharacter>(GetControlledPawn()))
    {
        if (UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement())
        {
            MoveComp->bUseControllerDesiredRotation = false;
            MoveComp->RotationRate = FRotator::ZeroRotator;
        }
    }
}

bool USFTurnInPlaceComponent::CanTurnInPlace() const
{
    APawn* Pawn = GetControlledPawn();
    if (!Pawn || !Pawn->HasAuthority()) return false;
    
    
    if (bIsTurning) return false;
    if (!GetTargetActor()) return false;
    
    if (Pawn->GetVelocity().Size2D() > 10.f) return false;
    
    if (UCharacterMovementComponent* MoveComp = Cast<UCharacterMovementComponent>(Pawn->GetMovementComponent()))
    {
        if (MoveComp->IsFalling()) return false; 
        if (MoveComp->GetCurrentAcceleration().Size2D() > 0.f) return false;
    }
    
    UAbilitySystemComponent* ASC = GetASC();
    if (!ASC) return false;

    if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_UsingAbility)) return false;
    if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Attacking)) return false;
    if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_TurningInPlace)) return false;

    if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Stunned) ||
        ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Knockdown) ||
        ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
    {
        return false;
    }

    return true;
}