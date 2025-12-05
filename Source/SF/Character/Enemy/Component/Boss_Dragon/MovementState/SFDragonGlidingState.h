// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SFDragonMovementStateBase.h"
#include "SFDragonGlidingState.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFDragonGlidingState : public USFDragonMovementStateBase
{
	GENERATED_BODY()
	
public:
	// Factory Method
	static USFDragonGlidingState* CreateState(UObject* Outer, USFDragonMovementComponent* OwnerComponent);

	virtual void EnterState() override;
	virtual void UpdateState(float DeltaTime) override;
	virtual void ExitState() override;

	virtual FGameplayTag GetType() const override;

protected:
	void UpdateGliding(float DeltaTime);
};
