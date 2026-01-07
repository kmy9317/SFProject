#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "SFInventoryManagerComponent.generated.h"


struct FSFSavedItemSlot;
class USFItemDefinition;
class USFItemInstance;
class USFEquipmentManagerComponent;
class USFInventoryManagerComponent;
struct FSFItemRarityProbability;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnInventoryEntryChanged, int32 /*SlotIndex*/, USFItemInstance*, int32 /*ItemCount*/);

USTRUCT(BlueprintType)
struct FSFInventoryEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

public:
    USFItemInstance* GetItemInstance() const { return ItemInstance; }
    int32 GetItemCount() const { return ItemCount; }
    bool IsEmpty() const { return ItemInstance == nullptr; }

private:
    friend struct FSFInventoryList;
    friend class USFInventoryManagerComponent;

    void Set(USFItemInstance* InItemInstance, int32 InItemCount);
    void AddCount(int32 InCount);
    void RemoveCount(int32 InCount);
    USFItemInstance* Clear();

    UPROPERTY()
    TObjectPtr<USFItemInstance> ItemInstance;

    UPROPERTY()
    int32 ItemCount = 0;
};

USTRUCT(BlueprintType)
struct FSFInventoryList : public FFastArraySerializer
{
    GENERATED_BODY()

public:
    FSFInventoryList() : InventoryManager(nullptr) {}
    FSFInventoryList(USFInventoryManagerComponent* InOwnerComponent) : InventoryManager(InOwnerComponent) {}

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams);
    void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);

    const TArray<FSFInventoryEntry>& GetAllEntries() const { return Entries; }

private:
    friend class USFInventoryManagerComponent;

    void BroadcastChangedMessage(int32 SlotIndex);

    UPROPERTY()
    TArray<FSFInventoryEntry> Entries;

    UPROPERTY(NotReplicated)
    TObjectPtr<USFInventoryManagerComponent> InventoryManager;
};

template<>
struct TStructOpsTypeTraits<FSFInventoryList> : public TStructOpsTypeTraitsBase2<FSFInventoryList>
{
    enum { WithNetDeltaSerializer = true };
};

UCLASS(BlueprintType, Blueprintable)
class SF_API USFInventoryManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USFInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    void SaveToData(TArray<FSFSavedItemSlot>& OutSlots) const;
    void RestoreFromData(const TArray<FSFSavedItemSlot>& InSlots);

protected:
    virtual void InitializeComponent() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
    virtual void ReadyForReplication() override;

public:
    // ========== 검증 함수 ==========
    int32 CanAddItem(int32 ItemID, const FGameplayTag& RarityTag, int32 ItemCount, TArray<int32>& OutSlotIndices, TArray<int32>& OutCounts) const;

    bool CanRemoveItem(int32 ItemID, int32 ItemCount, TArray<int32>& OutSlotIndices, TArray<int32>& OutCounts) const;

    // ========== 안전한 조작 함수 (서버, 검증O) ==========

    // 픽업, 상자 보상, 드롭 결과 추가 (이미 생성된 인스턴스)
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
    int32 TryAddExistingItem(USFItemInstance* ItemInstance, int32 ItemCount);
    
    // 지정된 등급으로 아이템 추가(상점 구매, 퀘스트 보상 등)
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
    int32 TryAddItem(int32 ItemID, const FGameplayTag& RarityTag, int32 ItemCount);
    
    // 간단한 보상 지급 (드롭 테이블 없이)
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
    int32 TryAddItemWithRandomRarity(int32 ItemID, float LuckValue, int32 ItemCount);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
    int32 TryAddItemByClassWithRandomRarity(TSubclassOf<USFItemDefinition> ItemClass, float LuckValue, int32 ItemCount);

    // 아이템 제거
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
    bool TryRemoveItem(int32 ItemID, int32 ItemCount);
    
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
    bool TryRemoveItemAt(int32 SlotIndex, int32 ItemCount);
    
    // ========== 조회 함수 ==========
    UFUNCTION(BlueprintPure, Category = "Inventory")
    USFItemInstance* GetItemInstance(int32 SlotIndex) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetItemCount(int32 SlotIndex) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    bool IsSlotEmpty(int32 SlotIndex) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetTotalCountByID(int32 ItemID) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetTotalCountByClass(TSubclassOf<USFItemDefinition> ItemClass) const;

    // UI 및 스택 불가 아이템에 대해 빈 공간 체크 함수로 사용
    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 FindFirstEmptySlot() const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetSlotCount() const { return SlotCount; }

    const TArray<FSFInventoryEntry>& GetAllEntries() const { return InventoryList.GetAllEntries(); }

public:
    FOnInventoryEntryChanged OnInventoryEntryChanged;

private:
    friend class USFItemManagerComponent;

    // 장비 -> 인벤토리 이동시 
    void AddItemInternal(int32 SlotIndex, USFItemInstance* ItemInstance, int32 ItemCount);
    USFItemInstance* RemoveItemInternal(int32 SlotIndex, int32 ItemCount);

    bool IsValidSlotIndex(int32 SlotIndex) const { return SlotIndex >= 0 && SlotIndex < SlotCount; }

private:
    UPROPERTY(Replicated)
    FSFInventoryList InventoryList;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    int32 SlotCount = 20;
};