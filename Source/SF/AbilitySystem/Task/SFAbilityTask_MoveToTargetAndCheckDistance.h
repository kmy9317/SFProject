// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Abilities/Tasks/AbilityTask_MoveToLocation.h"
#include "SFAbilityTask_MoveToTargetAndCheckDistance.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMoveTaskFinishedDelegate,bool, bReachedTarget);

/**
 * 
 */
UCLASS()
class SF_API USFAbilityTask_MoveToTargetAndCheckDistance : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FMoveTaskFinishedDelegate OnFinished;


	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (DefaultToSelf = "OwningAbility"))
	static USFAbilityTask_MoveToTargetAndCheckDistance* MoveToTargetAndCheckDistance(
		UGameplayAbility* OwningAbility,
		AActor* Target,
		float StopDistance = 0.0f,
		float Duration = 0.0f,
		UCurveFloat* SpeedCurve = nullptr
	);
protected:
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	void FinishTask(bool bSuccess);
	virtual void OnDestroy(bool bInOwnerFinished) override;


private:
	UPROPERTY()
	TObjectPtr<AActor> Target;

	FVector StartLocation;
	FVector MoveDirection;
	
	float TotalDistance;
	float StopDistance;
	float Duration;
	float StartTime;
	
	UPROPERTY()
	TObjectPtr<UCurveFloat> SpeedCurve;
	
	bool bReachedTarget = false;
	bool bFinished = false; 

};
