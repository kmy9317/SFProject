// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "SFBTS_UpdateTargetData.generated.h"

class USFDragonCombatComponent;
class USFEnemyCombatComponent;
/**
 * 
 */
UCLASS()
class SF_API USFBTS_UpdateTargetData : public UBTService_BlackboardBase
{
	GENERATED_BODY()

public:
	USFBTS_UpdateTargetData();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;



protected:
	UPROPERTY(EditAnywhere, Category= "Blackboard")
	FBlackboardKeySelector DistanceKey;

	UPROPERTY(EditAnywhere, Category= "Blackboard")
	FBlackboardKeySelector AngleKey;

	UPROPERTY(EditAnywhere, Category= "Blackboard")
	FBlackboardKeySelector ZoneKey;

	
private:
	UPROPERTY()
	USFDragonCombatComponent* CombatComponent;
	
	
	
};
