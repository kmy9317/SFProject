// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/StateMachine/State/SFState.h"
#include "BossPhaseState.generated.h"

class USFGameplayAbility;
struct  FGameplayAbilitySpecHandle;
/**
 * 
 */
UCLASS()
class SF_API UBossPhaseState : public USFState
{
	GENERATED_BODY()


public:
	virtual void OnEnter_Implementation() override;
	virtual void OnExit_Implementation() override;

protected:
	void GivePhaseAbilities();
	void ClearPhaseAbilities();
protected:
	UPROPERTY(EditDefaultsOnly, Category = "SF|BossPhaseAbility")
	TArray<TSubclassOf<USFGameplayAbility>> PhaseAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "SF|BossPhaseBT")
	FGameplayTag BehaviourTag;

	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedPhaseAbilityHandles;
};
