#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/PawnComponent.h"
#include "SFEquipmentComponent.generated.h"

class ASFCharacterBase;
class USFEquipmentInstance;
class USFEquipmentDefinition;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFEquipmentComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	

	static USFEquipmentComponent* FindEquipmentComponent(AActor* OwnerActor){ return (OwnerActor ? OwnerActor->FindComponentByClass<USFEquipmentComponent>() : nullptr); }

	void EquipItem(USFEquipmentDefinition* EquipmentDefinition);

	// 슬롯 태그로 장비 제거
	void UnequipItem(FGameplayTag EquipmentSlotTag);

	// Instance로 직접 장비 제거
	void UnequipItemByInstance(USFEquipmentInstance* EquipmentInstance);

	UFUNCTION(BlueprintPure, Category = "Equipment")
	const TArray<USFEquipmentInstance*>& GetEquippedItems() const { return EquipmentInstances; }

	void InitializeEquipment();

	virtual void BeginPlay() override;
	

protected:

	virtual USFEquipmentInstance* FindEquipmentInstance(FGameplayTag EquipmentTag) const;

	virtual USFEquipmentInstance* FindEquipmentInstanceBySlot(FGameplayTag SlotTag) const;

protected:
	UPROPERTY()
	TArray<TObjectPtr<USFEquipmentInstance>> EquipmentInstances;

	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TArray<TObjectPtr<USFEquipmentDefinition>> DefaultEquipmentDefinitions;
	
	
};