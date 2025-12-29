// Fill out your copyright notice in the Description page of Project Settings.


#include "SFDragonController.h"

#include "SFDragonCombatComponent.h"


// Sets default values
ASFDragonController::ASFDragonController()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CombatComponent = CreateDefaultSubobject<USFDragonCombatComponent>(TEXT("CombatComponent"));
	CombatComponent->SetIsReplicated(true);
	
}

void ASFDragonController::InitializeAIController()
{
	UE_LOG(LogTemp, Warning, TEXT("=== Dragon InitializeAIController START ==="));

	Super::InitializeAIController();
	

	if (CombatComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Dragon: CombatComponent found, initializing..."));
		CombatComponent->InitializeCombatComponent();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Dragon: CombatComponent is NULL!"));
	}


}


