#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ControllerComponent.h"
#include "SFItemManagerComponent.generated.h"

class ASFPickupableItemBase;
class USFItemInstance;
class USFInventoryManagerComponent;
class USFQuickbarComponent;

UENUM(BlueprintType)
enum class ESFItemSlotType : uint8
{
	Inventory,
	Quickbar
};

USTRUCT(BlueprintType)
struct FSFItemSlotHandle
{
	GENERATED_BODY()

	FSFItemSlotHandle() : SlotType(ESFItemSlotType::Inventory), SlotIndex(INDEX_NONE) {}
	FSFItemSlotHandle(ESFItemSlotType InType, int32 InIndex) : SlotType(InType), SlotIndex(InIndex) {}

	UPROPERTY(BlueprintReadWrite, Category = "Item")
	ESFItemSlotType SlotType = ESFItemSlotType::Inventory;

	UPROPERTY(BlueprintReadWrite, Category = "Item")
	int32 SlotIndex = INDEX_NONE;

	bool IsValid() const { return SlotIndex != INDEX_NONE; }
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFItemManagerComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	USFItemManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	// ========== 통합 Server RPC ==========

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Item")
	void Server_MoveItem(FSFItemSlotHandle FromSlot, FSFItemSlotHandle ToSlot);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Item")
	void Server_UseItem(FSFItemSlotHandle Slot);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Item")
	void Server_DropItem(FSFItemSlotHandle Slot);

public:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Item")
	bool TryPickupItem(ASFPickupableItemBase* PickupableItem);

	// 슬롯에서 아이템 소모 (Cost에서 호출)
	void ConsumeFromSlot(const FSFItemSlotHandle& Slot, int32 Count = 1);

protected:

	void UseConsumableItem(USFItemInstance* ItemInstance, const class USFItemFragment_Consumable* ConsumeFrag, const FSFItemSlotHandle& Slot);

	bool IsValidSlot(const FSFItemSlotHandle& Slot) const;
	bool IsSlotEmpty(const FSFItemSlotHandle& Slot) const;
	bool GetSlotInfo(const FSFItemSlotHandle& Slot, USFItemInstance*& OutInstance, int32& OutCount) const;
	void RemoveFromSlot(const FSFItemSlotHandle& Slot, int32 Count);
	void AddToSlot(const FSFItemSlotHandle& Slot, USFItemInstance* Instance, int32 Count);

	USFInventoryManagerComponent* GetInventoryManager() const;
	USFQuickbarComponent* GetQuickbarComponent() const;
	class UAbilitySystemComponent* GetAbilitySystemComponent() const;

	// ========== 내부 헬퍼 ==========

	bool SpawnDroppedItem(USFItemInstance* ItemInstance, int32 ItemCount);
	FGameplayTag GetConsumeAbilityTriggerTag(const class USFItemFragment_Consumable* ConsumeFrag) const;
};
