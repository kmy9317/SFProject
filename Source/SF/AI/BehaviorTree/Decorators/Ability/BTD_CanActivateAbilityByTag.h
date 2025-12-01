// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlueprintBase.h"
#include "BTD_CanActivateAbilityByTag.generated.h"

/**
 * 
 */
// 이거 개인별 메모리 공간 할당을 위해서 
struct FBTCanActivateAbilityMemory
{
	bool bLastResult = false;
	float TimeSinceLastCheck = 0.f;
};

UCLASS()
class SF_API UBTD_CanActivateAbilityByTag : public UBTDecorator_BlueprintBase
{
	GENERATED_BODY()

public:
	UBTD_CanActivateAbilityByTag();
protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;

protected:
	UPROPERTY(EditAnywhere, Category= "Ability")
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, Category = "Performance", meta = (ClampMin = "0.0"))
	float CheckInterval = 0.1f;

	
};
