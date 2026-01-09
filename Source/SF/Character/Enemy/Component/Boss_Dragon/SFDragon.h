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
	ASFDragon(const FObjectInitializer& ObjectInitializer);
	
	virtual void InitializeComponents() override;
	
	virtual TArray<FName> GetLockOnSockets() const override;

protected:

	virtual void OnAbilitySystemInitialized() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|LockOn")
	TArray<FName> LockOnSockets;
};
