// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "Character/Enemy/SFEnemyData.h"
#include "SFEnemyController.generated.h"


class UBehaviorTreeComponent;
class UBlackboardComponent;
class UBehaviorTree;

UCLASS()
class SF_API ASFEnemyController : public AAIController
{
	GENERATED_BODY()

public:
	
	ASFEnemyController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void SetBehaviourContainer(FSFBehaviourWrapperContainer InBehaviorTreeContainer){ BehaviorTreeContainer = InBehaviorTreeContainer; }
	

protected:

	virtual void PreInitializeComponents() override;
	
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void OnPossess(APawn* InPawn) override;

	virtual void OnUnPossess() override;



#pragma region BehaviorTree
protected:
	void ChangeBehaviorTree(FGameplayTag GameplayTag);
	
	void StopBehaviorTree();

	 void SetBehaviorTree(UBehaviorTree* BehaviorTree);

	void BindingStateMachine(const APawn* InPawn);
	
	void UnBindingStateMachine();
	
protected:
	UPROPERTY()
	UBehaviorTreeComponent* CachedBehaviorTreeComponent;
    
	UPROPERTY()
	UBlackboardComponent* CachedBlackboardComponent;

	UPROPERTY()
	FSFBehaviourWrapperContainer BehaviorTreeContainer; 


#pragma endregion 

	

	
};
