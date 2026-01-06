#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ControllerComponent.h"
#include "SFItemManagerComponent.generated.h"


class ASFPickupableItemBase;
class USFItemInstance;
class USFInventoryManagerComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFItemManagerComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	USFItemManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	// ========== 인벤토리 내 이동 ==========
    
	// 슬롯 간 이동/병합
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Item")
	void Server_MoveItem(int32 FromSlotIndex, int32 ToSlotIndex);

	// ========== 아이템 사용 ==========
    
	// 소모품 사용
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Item")
	void Server_UseItem(int32 SlotIndex);

	// ========== 아이템 드롭 ==========
    
	// 인벤토리에서 월드로 드롭
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Item")
	void Server_DropItem(int32 SlotIndex, int32 DropCount);

	// ========== 아이템 픽업 (서버 전용) ==========
    
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Item")
	bool TryPickupItem(ASFPickupableItemBase* PickupableItem);

protected:
	// 내부 헬퍼
	USFInventoryManagerComponent* GetInventoryManager() const;
	class UAbilitySystemComponent* GetAbilitySystemComponent() const;

	// 드롭 아이템 스폰
	bool SpawnDroppedItem(USFItemInstance* ItemInstance, int32 ItemCount);

	// 어빌리티 트리거 태그 결정
	FGameplayTag GetAbilityTriggerTag(const class USFItemFragment_Consumable* ConsumeFrag) const;
};
