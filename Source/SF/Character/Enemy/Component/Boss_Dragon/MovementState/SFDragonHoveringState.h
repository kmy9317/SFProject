// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SFDragonMovementStateBase.h"
#include "SFDragonHoveringState.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFDragonHoveringState : public USFDragonMovementStateBase
{
	GENERATED_BODY()

public:
	
	static USFDragonHoveringState* CreateState(UObject* Outer, USFDragonMovementComponent* OwnerComponent);

	virtual void EnterState() override;
	virtual void UpdateState(float DeltaTime) override;
	virtual void ExitState() override;

	virtual FGameplayTag GetType() const override;

	protected:
		void UpdateHovering(float DeltaTime);

	protected:
		UPROPERTY(EditAnywhere, Category = "Hovering")
		FVector PivotLocation = FVector::ZeroVector;

		UPROPERTY(EditAnywhere, Category = "Hovering")
		FVector HoveringDirection  = FVector(0.0f,0.0f,1.0f);;

		UPROPERTY(EditAnywhere, Category = "Hovering")
		FVector HoveringLocation = FVector::ZeroVector;
	
};
