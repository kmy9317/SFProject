#include "SFDragonController.h"
#include "AbilitySystemComponent.h"
#include "AI/Controller/SFTurnInPlaceComponent.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFDragonController)

ASFDragonController::ASFDragonController()
{
	PrimaryActorTick.bCanEverTick = true;

	
	CombatComponent = CreateDefaultSubobject<USFDragonCombatComponent>(TEXT("CombatComponent"));
	CombatComponent->SetIsReplicated(true);
	
	TurnInPlaceComponent = CreateDefaultSubobject<USFTurnInPlaceComponent>(TEXT("TurnInPlaceComponent"));
	
	RotationInterpSpeed = 5.f;
}
 
void ASFDragonController::InitializeAIController()
{
	Super::InitializeAIController();

	if (TurnInPlaceComponent)
	{
		TurnInPlaceComponent->InitializeTurnInPlaceComponent();
	}
	SetRotationMode(EAIRotationMode::MovementDirection);
}

bool ASFDragonController::IsTurningInPlace() const
{
	if (TurnInPlaceComponent)
	{
		return TurnInPlaceComponent->IsTurning();
	}
	return false;
}

float ASFDragonController::GetTurnThreshold() const
{
	if (TurnInPlaceComponent)
	{
		return TurnInPlaceComponent->GetTurnThreshold();
	}
	return 45.0f;
}

bool ASFDragonController::ShouldRotateActorByController() const
{
	return false;
}

void ASFDragonController::SetRotationMode(EAIRotationMode NewMode)
{
	if (CurrentRotationMode == NewMode) return;

	ACharacter* Char = Cast<ACharacter>(GetPawn());
	if (!Char) return;

	UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement();
	if (!MoveComp) return;

	CurrentRotationMode = NewMode;
	
	const float DragonRotationRate = 120.0f; 

	switch (NewMode)
	{
	case EAIRotationMode::MovementDirection:
		MoveComp->bOrientRotationToMovement = true; 
		MoveComp->bUseControllerDesiredRotation = false; 
		MoveComp->RotationRate = FRotator(0.f, DragonRotationRate, 0.f); 
		break;

	case EAIRotationMode::ControllerYaw:
		MoveComp->bOrientRotationToMovement = false; 
		MoveComp->bUseControllerDesiredRotation = false; 
		MoveComp->RotationRate = FRotator(0.f, DragonRotationRate, 0.f); 
		break;

	case EAIRotationMode::None: 
		MoveComp->bOrientRotationToMovement = false;
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->RotationRate = FRotator::ZeroRotator;
		break;
	}
}