#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "BehaviorTree/BTTaskNode.h"
#include "SFBTT_MoveStep.generated.h"

UCLASS()
class SF_API USFBTT_MoveStep : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USFBTT_MoveStep();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// Ability 종료 콜백
	UFUNCTION()
	void OnAbilityEnded(const FAbilityEndedData& EndedData, TWeakObjectPtr<UBehaviorTreeComponent> OwnerCompPtr);

	UFUNCTION()
	void Cleanup(UBehaviorTreeComponent* OwnerComp);

public:
	UPROPERTY(EditAnywhere, Category="Ability")
	TSubclassOf<class USFGA_MoveStep> AbilityClass;

	UPROPERTY(EditAnywhere, Category= "BlackBoard")
	FBlackboardKeySelector TargetKey;

private:
	FGameplayAbilitySpecHandle RunningAbilityHandle;
	FDelegateHandle AbilityEndedHandle;
};
