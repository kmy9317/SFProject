// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Enemy/SFEnemy.h"
#include "SFDragon.generated.h"

class USFDragonMovementComponent;

UCLASS()
class SF_API ASFDragon : public ASFEnemy
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASFDragon();

	virtual void InitializeMovementComponent() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MovementComponent")
	TObjectPtr<USFDragonMovementComponent> DragonMovementComponent;
};
