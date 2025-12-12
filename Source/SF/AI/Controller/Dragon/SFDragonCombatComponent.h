// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "SFDragonCombatComponent.generated.h"


class ASFCharacterBase;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFDragonCombatComponent : public USFEnemyCombatComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USFDragonCombatComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitializeCombatComponent() override;

	UFUNCTION()
	void AddThreat( float ThreatValue, AActor* Actor);

	AActor* GetHighestThreatActor();

	
	

protected:
	UPROPERTY()
	TMap<AActor*, float> ThreatMap;
	

};
