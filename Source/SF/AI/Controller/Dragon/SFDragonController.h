// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/Controller/SFBaseAIController.h"
#include "SFDragonController.generated.h"

UCLASS()
class SF_API ASFDragonController : public ASFBaseAIController
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASFDragonController();

	virtual void InitializeAIController() override;

};
