#include "SFDragonController.h"
#include "AbilitySystemComponent.h"
#include "AI/Controller/SFTurnInPlaceComponent.h"

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

	if (CombatComponent)
	{
		CombatComponent->InitializeCombatComponent();
	}
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