// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "SFBTTask_ActivateChargeAbility.generated.h"

class UAbilitySystemComponent;
struct FAbilityEndedData;
/**
 * SFBTTask_ActivateChargeAbility
 * 이건 ExecuteAbilityByTag과 비슷하지만, Key값이 아닌 Tag를 직접 입력해서 실행 시킨다 Ex(근접 이동과 같은것)
 */
UCLASS()
class SF_API USFBTTask_ActivateAbilityByTag : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USFBTTask_ActivateAbilityByTag(const FObjectInitializer& ObjectInitializer);

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;

	virtual void OnTaskFinished(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		EBTNodeResult::Type TaskResult
	) override;
	void CleanupDelegates(UBehaviorTreeComponent& OwnerComp);

	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp,uint8* NodeMemory) override;


private:
	UAbilitySystemComponent* GetASC(UBehaviorTreeComponent& OwnerComp) const;

	void OnAbilityEnded(const FAbilityEndedData& EndedData);

private:

	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

	FGameplayAbilitySpecHandle ExecutingAbilityHandle;


	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	FDelegateHandle AbilityEndedHandle;
};

