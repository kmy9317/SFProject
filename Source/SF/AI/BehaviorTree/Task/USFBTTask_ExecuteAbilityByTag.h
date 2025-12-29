// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "GameplayTagContainer.h"
#include "USFBTTask_ExecuteAbilityByTag.generated.h"

class UAbilitySystemComponent;
struct FAbilityEndedData;

UCLASS()
class SF_API UUSFBTTask_ExecuteAbilityByTag : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UUSFBTTask_ExecuteAbilityByTag(const FObjectInitializer& ObjectInitializer);

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	virtual void OnTaskFinished(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		EBTNodeResult::Type TaskResult) override;

	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp,uint8* NodeMemory) override;
	
	void CleanupDelegates(UBehaviorTreeComponent& OwnerComp);

private:
	UAbilitySystemComponent* GetASC(UBehaviorTreeComponent& OwnerComp) const;

	void OnAbilityEnded(const FAbilityEndedData& EndedData);
	

	

private:
	FDelegateHandle AbilityEndedHandle;
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	UPROPERTY(EditAnywhere, Category = "Ability")
	FBlackboardKeySelector AbilityTagKey;

	UPROPERTY(EditAnywhere, Category = "Ability")
	FGameplayAbilitySpecHandle ExecutingAbilityHandle; 
};
