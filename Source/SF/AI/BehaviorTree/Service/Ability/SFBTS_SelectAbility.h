// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "SFBTS_SelectAbility.generated.h"

// 탐색할 어빌리티 태그를 가져와서 실행시킬 수있는지 가져와서 블랙보드에 저장
/**
 * 
 */
// SFBTS_SelectAbility.h
UCLASS()
class SF_API USFBTS_SelectAbility : public UBTService_BlackboardBase
{
	GENERATED_BODY()

public:
	USFBTS_SelectAbility();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    
	
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;

	UPROPERTY(EditAnywhere, Category = "Ability")
	FGameplayTagContainer AbilitySearchTags;


private:
	
	struct FBTSelectAbilityMemory
	{

		FGameplayTag LastSelectedAbilityTag;

		FBTSelectAbilityMemory() : LastSelectedAbilityTag() {}
	};
};