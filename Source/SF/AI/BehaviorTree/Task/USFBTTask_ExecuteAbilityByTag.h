// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"              
#include "AbilitySystemComponent.h"          
#include "USFBTTask_ExecuteAbilityByTag.generated.h"

/**
 * 
 */
UCLASS()
class SF_API UUSFBTTask_ExecuteAbilityByTag : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UUSFBTTask_ExecuteAbilityByTag(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
    
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
    
	UAbilitySystemComponent* GetASC(UBehaviorTreeComponent& OwnerComp) const;  
	void ReceiveTagChanged(FGameplayTag Tag, int32 NewCount);

public:
	UPROPERTY(EditAnywhere, Category= "Blackboard")
	FBlackboardKeySelector AbilityTagKey;

protected:
	FDelegateHandle EventHandle;
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
	
	FGameplayTag WatchedTag;
};
