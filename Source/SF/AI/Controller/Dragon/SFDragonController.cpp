// Fill out your copyright notice in the Description page of Project Settings.


#include "SFDragonController.h"

#include "SFDragonCombatComponent.h"


// Sets default values
ASFDragonController::ASFDragonController()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	CombatComponent = CreateDefaultSubobject<USFDragonCombatComponent>(TEXT("CombatComponent"));
	CombatComponent->SetIsReplicated(true);
	
}

void ASFDragonController::InitializeAIController()
{
	Super::InitializeAIController();
	if (CombatComponent)
	{
		CombatComponent->InitializeCombatComponent();
	}
	
}


