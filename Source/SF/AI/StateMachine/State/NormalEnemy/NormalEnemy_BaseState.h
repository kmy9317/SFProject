// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/StateMachine/State/SFState.h"
#include "NormalEnemy_BaseState.generated.h"

/**
 * 
 */
UCLASS()
class SF_API UNormalEnemy_BaseState : public USFState
{
	GENERATED_BODY()

public:
	virtual void OnEnter_Implementation() override;


private:
	UPROPERTY(EditDefaultsOnly, Category = "BehaviorTag",meta=(AllowPrivateAccess = "true"))
	FGameplayTag BehaviourTag;
};
