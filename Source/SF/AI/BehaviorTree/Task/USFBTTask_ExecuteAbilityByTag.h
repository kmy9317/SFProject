
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "GameplayTagContainer.h"
#include "USFBTTask_ExecuteAbilityByTag.generated.h"

class UAbilitySystemComponent;

UCLASS()
class SF_API UUSFBTTask_ExecuteAbilityByTag : public UBTTask_BlackboardBase
{
	GENERATED_BODY()


public:
	UUSFBTTask_ExecuteAbilityByTag(const FObjectInitializer& ObjectInitializer);

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	void CleanupDelegate(UBehaviorTreeComponent& OwnerComp);

private:
	UAbilitySystemComponent* GetASC(UBehaviorTreeComponent& OwnerComp) const;

	
	void OnTagChanged(FGameplayTag Tag, int32 NewCount);


	FDelegateHandle EventHandle;

	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	FGameplayTag WatchedTag;
	bool bFinished = false;

	UPROPERTY(EditAnywhere)
	FBlackboardKeySelector AbilityTagKey;
};
