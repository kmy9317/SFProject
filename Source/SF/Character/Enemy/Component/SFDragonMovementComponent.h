// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Character/Enemy/Component/SFEnemyMovementComponent.h"
#include "SFDragonMovementComponent.generated.h"



UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFDragonMovementComponent : public USFEnemyMovementComponent
{
	GENERATED_BODY()

public:
	USFDragonMovementComponent();

	// 초기화 
	virtual void InitializeMovementComponent() override;

	// 속도 
	virtual float GetMaxSpeed() const override;


	UFUNCTION(BlueprintCallable, Category = "Dragon Movement")
	void SetSprinting(bool bNewSprinting);
	
	UFUNCTION(BlueprintCallable, Category = "Dragon Movement")
	void SetFlyingMode(bool bFly);

protected:
	virtual void InternalDisableMovement() override;

protected:
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dragon Movement")
	float DragonWalkSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dragon Movement")
	float DragonRunSpeed = 1300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dragon Movement")
	float DragonFlySpeed = 1500.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dragon Movement")
	float HeavyMaxAcceleration = 800.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dragon Movement")
	float HeavyBrakingDeceleration = 1000.f;

;
private:
	uint8 bIsSprinting : 1;
	
};