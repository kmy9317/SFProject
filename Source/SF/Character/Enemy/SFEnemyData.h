// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Character/SFPawnData.h"
#include "SFEnemyData.generated.h"

class USFState;
class UBehaviorTree;

USTRUCT(BlueprintType)
struct FSFBehaviourWrapper
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag BehaviourTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UBehaviorTree> Behaviour;
};

USTRUCT(BlueprintType)
struct FSFBehaviourWrapperContainer
{
	GENERATED_BODY()
	
	UBehaviorTree* GetBehaviourTree(const FGameplayTag& Tag) const;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FSFBehaviourWrapper> Behaviours;
};

USTRUCT(BlueprintType)
struct FSTStateWrapper
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag StateTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<USFState> StateClass;
};

USTRUCT(BlueprintType)
struct FSTStateWrapperContainer
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FSTStateWrapper> States;
};

/**
 * 
 */
UCLASS()
class SF_API USFEnemyData : public USFPawnData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Type")
	FGameplayTag EnemyType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behaviour")
	FSFBehaviourWrapperContainer BehaviourContainer;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|State")
	FSTStateWrapperContainer StateContainer;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|State")
	FGameplayTag DefaultStateTag; 
	
};