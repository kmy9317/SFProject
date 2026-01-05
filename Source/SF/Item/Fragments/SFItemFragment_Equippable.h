#pragma once

#include "CoreMinimal.h"
#include "Item/SFItemDefinition.h"
#include "SFItemFragment_Equippable.generated.h"

class USFAbilitySet;

UCLASS()
class SF_API USFItemFragment_Equippable : public USFItemFragment
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equip", meta = (Categories = "Equipment.Slot"))
	FGameplayTag EquipSlotTag;
	
	UPROPERTY(EditDefaultsOnly, Category="SF|AbilitySet")
	TObjectPtr<const USFAbilitySet> BaseAbilitySet;
};
