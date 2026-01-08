#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ControllerComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "SFQuickbarComponent.generated.h"

struct FSFSavedItemSlot;
class USFItemInstance;
class USFItemDefinition;
class USFInventoryManagerComponent;
class USFQuickbarComponent;
struct FSFQuickbarList;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnQuickbarEntryChanged, int32 /*SlotIndex*/, USFItemInstance*, int32 /*ItemCount*/);

USTRUCT(BlueprintType)
struct FSFQuickbarEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

public:
    USFItemInstance* GetItemInstance() const { return ItemInstance; }
    int32 GetItemCount() const { return ItemCount; }
    bool IsEmpty() const { return ItemInstance == nullptr; }

private:
    friend struct FSFQuickbarList;
    friend class USFQuickbarComponent;

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
struct FSFQuickbarList : public FFastArraySerializer
{
    GENERATED_BODY()

public:
    FSFQuickbarList() : QuickbarComponent(nullptr) {}
    FSFQuickbarList(USFQuickbarComponent* InOwnerComponent) : QuickbarComponent(InOwnerComponent) {}

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams);
    void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);

    const TArray<FSFQuickbarEntry>& GetAllEntries() const { return Entries; }

private:
    friend class USFQuickbarComponent;

    void BroadcastChangedMessage(int32 SlotIndex);

    UPROPERTY()
    TArray<FSFQuickbarEntry> Entries;

    UPROPERTY(NotReplicated)
    TObjectPtr<USFQuickbarComponent> QuickbarComponent;
};

template<>
struct TStructOpsTypeTraits<FSFQuickbarList> : public TStructOpsTypeTraitsBase2<FSFQuickbarList>
{
    enum { WithNetDeltaSerializer = true };
};

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SF_API USFQuickbarComponent : public UControllerComponent
{
    GENERATED_BODY()

public:
    USFQuickbarComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    void SaveToData(TArray<FSFSavedItemSlot>& OutSlots) const;
    void RestoreFromData(const TArray<FSFSavedItemSlot>& InSlots);

protected:
    virtual void InitializeComponent() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
    virtual void ReadyForReplication() override;

public:

    // ========== 조회 함수 ==========

    UFUNCTION(BlueprintPure, Category = "Quickbar")
    USFItemInstance* GetItemInstance(int32 SlotIndex) const;

    UFUNCTION(BlueprintPure, Category = "Quickbar")
    int32 GetItemCount(int32 SlotIndex) const;

    UFUNCTION(BlueprintPure, Category = "Quickbar")
    bool IsSlotEmpty(int32 SlotIndex) const;

    UFUNCTION(BlueprintPure, Category = "Quickbar")
    bool IsValidSlotIndex(int32 SlotIndex) const { return SlotIndex >= 0 && SlotIndex < SlotCount; }

    UFUNCTION(BlueprintPure, Category = "Quickbar")
    int32 GetSlotCount() const { return SlotCount; }

    const TArray<FSFQuickbarEntry>& GetAllEntries() const { return QuickbarList.GetAllEntries(); }

    UFUNCTION(BlueprintCallable, Category = "Quickbar")
    bool TryAddItem(int32 SlotIndex, USFItemInstance* ItemInstance, int32 Count);

    UFUNCTION(BlueprintCallable, Category = "Quickbar")
    bool TryRemoveItem(int32 SlotIndex, int32 Count);

public:
    FOnQuickbarEntryChanged OnQuickbarEntryChanged;

private:
    friend class USFItemManagerComponent;

    void AddItemInternal(int32 SlotIndex, USFItemInstance* ItemInstance, int32 ItemCount);
    USFItemInstance* RemoveItemInternal(int32 SlotIndex, int32 ItemCount);

private:
    UPROPERTY(Replicated)
    FSFQuickbarList QuickbarList;

    UPROPERTY(EditDefaultsOnly, Category = "Quickbar")
    int32 SlotCount = 4;
};
