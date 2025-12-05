// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SFDragonMovementStateBase.h"
#include "USFDragonDisableState.generated.h"

/**
 * 드래곤 비활성화 상태 (조작 불가)
 */
UCLASS()
class SF_API USFDragonDisabledState : public USFDragonMovementStateBase
{
	GENERATED_BODY()

public:
	// Factory Method
	static USFDragonDisabledState* CreateState(UObject* Outer, USFDragonMovementComponent* OwnerComponent);

	virtual void EnterState() override;
	virtual void UpdateState(float DeltaTime) override;
	virtual void ExitState() override;

	virtual FGameplayTag GetType() const override;
};
