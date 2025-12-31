// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "SFBTTask_SelectAbilityByTag.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFBTTask_SelectAbilityByTag : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:

	USFBTTask_SelectAbilityByTag();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;


protected:
	UPROPERTY(EditAnywhere, Category = "Ability")
	FGameplayTagContainer AbilitySearchTags;

	UPROPERTY(EditAnywhere, Category = "BlackBoard", meta=(AllowNone = "true"))
	FBlackboardKeySelector MinRangeKey;

	UPROPERTY(EditAnywhere, Category = "BlackBoard", meta=(AllowNone = "true"))
	FBlackboardKeySelector MaxRangeKey;
	
};
