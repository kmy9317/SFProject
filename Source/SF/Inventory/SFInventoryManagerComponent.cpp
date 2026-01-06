#include "SFInventoryManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"
#include "Item/SFItemData.h"
#include "Item/SFItemDefinition.h"
#include "Item/SFItemInstance.h"
#include "Item/SFItemRarityConfig.h"

void FSFInventoryEntry::Set(USFItemInstance* InItemInstance, int32 InItemCount)
{
    ItemInstance = InItemInstance;
    ItemCount = InItemCount;
}

void FSFInventoryEntry::AddCount(int32 InCount)
{
    ItemCount += InCount;
}

void FSFInventoryEntry::RemoveCount(int32 InCount)
{
    ItemCount = FMath::Max(0, ItemCount - InCount);
}

USFItemInstance* FSFInventoryEntry::Clear()
{
    USFItemInstance* RemovedInstance = ItemInstance;
    ItemInstance = nullptr;
    ItemCount = 0;
    return RemovedInstance;
}

//////////////////////////////////////////////////////////////////////////
// FSFInventoryList

bool FSFInventoryList::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
{
    return FFastArraySerializer::FastArrayDeltaSerialize<FSFInventoryEntry, FSFInventoryList>(Entries, DeltaParams, *this);
}

void FSFInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
    for (int32 Index : AddedIndices)
    {
        BroadcastChangedMessage(Index);
    }
}

void FSFInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
    for (int32 Index : ChangedIndices)
    {
        BroadcastChangedMessage(Index);
    }
}

void FSFInventoryList::BroadcastChangedMessage(int32 SlotIndex)
{
    if (InventoryManager && InventoryManager->OnInventoryEntryChanged.IsBound())
    {
        const FSFInventoryEntry& Entry = Entries[SlotIndex];
        InventoryManager->OnInventoryEntryChanged.Broadcast(SlotIndex, Entry.ItemInstance, Entry.ItemCount);
    }
}

//////////////////////////////////////////////////////////////////////////
// USFInventoryManagerComponent

USFInventoryManagerComponent::USFInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer), InventoryList(this)
{
    bWantsInitializeComponent = true;
    SetIsReplicatedByDefault(true);
}

void USFInventoryManagerComponent::InitializeComponent()
{
    Super::InitializeComponent();

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        InventoryList.Entries.SetNum(SlotCount);

        for (FSFInventoryEntry& Entry : InventoryList.Entries)
        {
            InventoryList.MarkItemDirty(Entry);
        }
    }
}

void USFInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, InventoryList);
}

bool USFInventoryManagerComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
    bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

    for (FSFInventoryEntry& Entry : InventoryList.Entries)
    {
        if (USFItemInstance* ItemInstance = Entry.ItemInstance)
        {
            bWroteSomething |= Channel->ReplicateSubobject(ItemInstance, *Bunch, *RepFlags);
        }
    }

    return bWroteSomething;
}

void USFInventoryManagerComponent::ReadyForReplication()
{
    Super::ReadyForReplication();

    if (IsUsingRegisteredSubObjectList())
    {
        for (const FSFInventoryEntry& Entry : InventoryList.Entries)
        {
            if (USFItemInstance* ItemInstance = Entry.ItemInstance)
            {
                AddReplicatedSubObject(ItemInstance);
            }
        }
    }
}

int32 USFInventoryManagerComponent::CanAddItem(int32 ItemID, const FGameplayTag& RarityTag, int32 ItemCount, TArray<int32>& OutSlotIndices, TArray<int32>& OutCounts) const
{
    OutSlotIndices.Reset();
    OutCounts.Reset();

    if (ItemID <= 0 || !RarityTag.IsValid() || ItemCount <= 0)
    {
        return 0;
    }

    const USFItemDefinition* ItemDef = USFItemData::Get().FindDefinitionById(ItemID);
    if (!ItemDef)
    {
        return 0;
    }

    int32 RemainingCount = ItemCount;
    const int32 MaxStack = ItemDef->MaxStackCount;
    const TArray<FSFInventoryEntry>& Entries = InventoryList.Entries;

    // 1. 기존 스택에 병합 시도 (MaxStack > 1인 경우)
    if (MaxStack > 1)
    {
        for (int32 i = 0; i < Entries.Num() && RemainingCount > 0; ++i)
        {
            const FSFInventoryEntry& Entry = Entries[i];
            if (!Entry.ItemInstance)
            {
                continue;
            }

            if (Entry.ItemInstance->GetItemID() != ItemID)
            {
                continue;
            }

            if (!Entry.ItemInstance->GetItemRarityTag().MatchesTagExact(RarityTag))
            {
                continue;
            }

            const int32 Space = MaxStack - Entry.ItemCount;
            if (Space > 0)
            {
                const int32 ToAdd = FMath::Min(Space, RemainingCount);
                OutSlotIndices.Add(i);
                OutCounts.Add(ToAdd);
                RemainingCount -= ToAdd;
            }
        }
    }

    // 2. 빈 슬롯에 새로 배치
    for (int32 i = 0; i < Entries.Num() && RemainingCount > 0; ++i)
    {
        if (Entries[i].IsEmpty())
        {
            const int32 ToAdd = FMath::Min(MaxStack, RemainingCount);
            OutSlotIndices.Add(i);
            OutCounts.Add(ToAdd);
            RemainingCount -= ToAdd;
        }
    }

    return ItemCount - RemainingCount;
}

bool USFInventoryManagerComponent::CanRemoveItem(int32 ItemID, int32 ItemCount, TArray<int32>& OutSlotIndices, TArray<int32>& OutCounts) const
{
    OutSlotIndices.Reset();
    OutCounts.Reset();

    if (ItemID <= 0 || ItemCount <= 0)
    {
        return false;
    }

    int32 RemainingCount = ItemCount;
    const TArray<FSFInventoryEntry>& Entries = InventoryList.Entries;

    // 뒤에서부터 검색 (LIFO)
    for (int32 i = Entries.Num() - 1; i >= 0 && RemainingCount > 0; --i)
    {
        const FSFInventoryEntry& Entry = Entries[i];
        if (!Entry.ItemInstance || Entry.ItemInstance->GetItemID() != ItemID)
        {
            continue;
        }

        const int32 ToRemove = FMath::Min(Entry.ItemCount, RemainingCount);
        OutSlotIndices.Add(i);
        OutCounts.Add(ToRemove);
        RemainingCount -= ToRemove;
    }

    return RemainingCount == 0;
}

int32 USFInventoryManagerComponent::TryAddExistingItem(USFItemInstance* ItemInstance, int32 ItemCount)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return 0;
    }

    if (!ItemInstance || ItemCount <= 0)
    {
        return 0;
    }

    TArray<int32> SlotIndices;
    TArray<int32> Counts;

    int32 AddableCount = CanAddItem(ItemInstance->GetItemID(), ItemInstance->GetItemRarityTag(), ItemCount, SlotIndices, Counts);
    if (AddableCount <= 0)
    {
        return 0;
    }

    for (int32 i = 0; i < SlotIndices.Num(); ++i)
    {
        AddItemInternal(SlotIndices[i], ItemInstance, Counts[i]);
    }

    return AddableCount;
}

int32 USFInventoryManagerComponent::TryAddItem(int32 ItemID, const FGameplayTag& RarityTag, int32 ItemCount)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return 0;
    }

    TArray<int32> SlotIndices;
    TArray<int32> Counts;

    const int32 AddableCount = CanAddItem(ItemID, RarityTag, ItemCount, SlotIndices, Counts);
    if (AddableCount <= 0)
    {
        return 0;
    }

    const USFItemData& ItemData = USFItemData::Get();

    for (int32 i = 0; i < SlotIndices.Num(); ++i)
    {
        const int32 SlotIndex = SlotIndices[i];
        const int32 Count = Counts[i];
        FSFInventoryEntry& Entry = InventoryList.Entries[SlotIndex];

        if (Entry.ItemInstance)
        {
            Entry.AddCount(Count);
        }
        else
        {
            const USFItemDefinition* ItemDef = ItemData.FindDefinitionById(ItemID);
            USFItemInstance* NewInstance = ItemData.CreateItemInstance(GetOwner(), ItemDef, RarityTag);

            Entry.Set(NewInstance, Count);

            if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
            {
                AddReplicatedSubObject(NewInstance);
            }
        }

        InventoryList.MarkItemDirty(Entry);
    }

    return AddableCount;
}

int32 USFInventoryManagerComponent::TryAddItemByClass(TSubclassOf<USFItemDefinition> ItemClass, const FGameplayTag& RarityTag, int32 ItemCount)
{
    if (!ItemClass)
    {
        return 0;
    }

    const int32 ItemID = USFItemData::Get().FindIdByDefinition(ItemClass.GetDefaultObject());
    return TryAddItem(ItemID, RarityTag, ItemCount);
}

int32 USFInventoryManagerComponent::TryAddItemWithRandomRarity(int32 ItemID, float LuckValue, int32 ItemCount)
{
    const USFItemData& ItemData = USFItemData::Get();

    // Luck 기반으로 등급 결정
    const USFItemRarityConfig* RarityConfig = ItemData.PickRandomRarity(LuckValue);
    if (!RarityConfig)
    {
        return 0;
    }

    return TryAddItem(ItemID, RarityConfig->RarityTag, ItemCount);
}

int32 USFInventoryManagerComponent::TryAddItemByClassWithRandomRarity(TSubclassOf<USFItemDefinition> ItemClass, float LuckValue, int32 ItemCount)
{
    if (!ItemClass)
    {
        return 0;
    }

    const int32 ItemID = USFItemData::Get().FindIdByDefinition(ItemClass.GetDefaultObject());
    return TryAddItemWithRandomRarity(ItemID, LuckValue, ItemCount);
}

bool USFInventoryManagerComponent::TryRemoveItem(int32 ItemID, int32 ItemCount)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return false;
    }

    TArray<int32> SlotIndices;
    TArray<int32> Counts;

    if (!CanRemoveItem(ItemID, ItemCount, SlotIndices, Counts))
    {
        return false;
    }

    for (int32 i = 0; i < SlotIndices.Num(); ++i)
    {
        RemoveItemInternal(SlotIndices[i], Counts[i]);
    }

    return true;
}

bool USFInventoryManagerComponent::TryRemoveItemByClass(TSubclassOf<USFItemDefinition> ItemClass, int32 ItemCount)
{
    if (!ItemClass)
    {
        return false;
    }

    const int32 ItemID = USFItemData::Get().FindIdByDefinition(ItemClass.GetDefaultObject());
    return TryRemoveItem(ItemID, ItemCount);
}

bool USFInventoryManagerComponent::TryRemoveItemAt(int32 SlotIndex, int32 ItemCount)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return false;
    }

    if (!IsValidSlotIndex(SlotIndex))
    {
        return false;
    }

    const FSFInventoryEntry& Entry = InventoryList.Entries[SlotIndex];
    if (Entry.IsEmpty() || Entry.ItemCount < ItemCount)
    {
        return false;
    }

    RemoveItemInternal(SlotIndex, ItemCount);
    return true;
}

void USFInventoryManagerComponent::AddItemInternal(int32 SlotIndex, USFItemInstance* ItemInstance, int32 ItemCount)
{
    if (!IsValidSlotIndex(SlotIndex))
    {
        return;
    }

    FSFInventoryEntry& Entry = InventoryList.Entries[SlotIndex];

    if (Entry.ItemInstance)
    {
        Entry.AddCount(ItemCount);
    }
    else
    {
        Entry.Set(ItemInstance, ItemCount);

        if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && ItemInstance)
        {
            AddReplicatedSubObject(ItemInstance);
        }
    }

    InventoryList.MarkItemDirty(Entry);
}

USFItemInstance* USFInventoryManagerComponent::RemoveItemInternal(int32 SlotIndex, int32 ItemCount)
{
    if (!IsValidSlotIndex(SlotIndex))
    {
        return nullptr;
    }

    FSFInventoryEntry& Entry = InventoryList.Entries[SlotIndex];
    USFItemInstance* ItemInstance = Entry.ItemInstance;

    Entry.RemoveCount(ItemCount);

    if (Entry.ItemCount <= 0)
    {
        USFItemInstance* RemovedInstance = Entry.Clear();

        if (IsUsingRegisteredSubObjectList() && RemovedInstance)
        {
            RemoveReplicatedSubObject(RemovedInstance);
        }
    }

    InventoryList.MarkItemDirty(Entry);
    return ItemInstance;
}

USFItemInstance* USFInventoryManagerComponent::GetItemInstance(int32 SlotIndex) const
{
    if (!IsValidSlotIndex(SlotIndex))
    {
        return nullptr;
    }

    return InventoryList.Entries[SlotIndex].ItemInstance;
}

int32 USFInventoryManagerComponent::GetItemCount(int32 SlotIndex) const
{
    if (!IsValidSlotIndex(SlotIndex))
    {
        return 0;
    }

    return InventoryList.Entries[SlotIndex].ItemCount;
}

bool USFInventoryManagerComponent::IsSlotEmpty(int32 SlotIndex) const
{
    if (!IsValidSlotIndex(SlotIndex))
    {
        return true;
    }

    return InventoryList.Entries[SlotIndex].IsEmpty();
}

int32 USFInventoryManagerComponent::GetTotalCountByID(int32 ItemID) const
{
    int32 TotalCount = 0;

    for (const FSFInventoryEntry& Entry : InventoryList.Entries)
    {
        if (Entry.ItemInstance && Entry.ItemInstance->GetItemID() == ItemID)
        {
            TotalCount += Entry.ItemCount;
        }
    }

    return TotalCount;
}

int32 USFInventoryManagerComponent::GetTotalCountByClass(TSubclassOf<USFItemDefinition> ItemClass) const
{
    if (!ItemClass)
    {
        return 0;
    }

    const int32 ItemID = USFItemData::Get().FindIdByDefinition(ItemClass.GetDefaultObject());
    return GetTotalCountByID(ItemID);
}

int32 USFInventoryManagerComponent::FindFirstEmptySlot() const
{
    for (int32 i = 0; i < InventoryList.Entries.Num(); ++i)
    {
        if (InventoryList.Entries[i].IsEmpty())
        {
            return i;
        }
    }

    return INDEX_NONE;
}
