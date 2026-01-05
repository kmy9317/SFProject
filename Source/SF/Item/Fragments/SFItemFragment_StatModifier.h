#pragma once

#include "CoreMinimal.h"
#include "Item/SFItemDefinition.h"
#include "Item/SFItemTypes.h"
#include "SFItemFragment_StatModifier.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFItemFragment_StatModifier : public USFItemFragment
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(USFItemInstance* Instance) const override;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stat")
	FSFTaggedRarityValues StatData;
};
