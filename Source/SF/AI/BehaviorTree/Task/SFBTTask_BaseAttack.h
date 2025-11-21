#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BTTaskNode.h"
#include "SFBTTask_BaseAttack.generated.h"

struct FGameplayEventData;

UCLASS()
class SF_API USFBTTask_BaseAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USFBTTask_BaseAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
    
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
    
	UAbilitySystemComponent* GetASC(UBehaviorTreeComponent& OwnerComp) const;  
	void OnAttackingTagChanged(FGameplayTag Tag, int32 NewCount);

public:
	UPROPERTY(EditAnywhere, Category = "Attack") 
	FGameplayTag AttackTag;

protected:
	FDelegateHandle EventHandle;
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;  
};