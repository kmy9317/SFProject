#include "SFQuickbarComponent.h"

#include "AbilitySystemComponent.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"
#include "Item/SFItemInstance.h"
#include "Inventory/SFInventoryManagerComponent.h"
#include "Player/Save/SFPersistentDataType.h"

void FSFQuickbarEntry::Set(USFItemInstance* InItemInstance, int32 InItemCount)
{
    ItemInstance = InItemInstance;
    ItemCount = InItemCount;
}

void FSFQuickbarEntry::AddCount(int32 InCount)
{
    ItemCount += InCount;
}

void FSFQuickbarEntry::RemoveCount(int32 InCount)
{
    ItemCount = FMath::Max(0, ItemCount - InCount);
}

USFItemInstance* FSFQuickbarEntry::Clear()
{
    USFItemInstance* RemovedInstance = ItemInstance;
    ItemInstance = nullptr;
    ItemCount = 0;
    return RemovedInstance;
}

bool FSFQuickbarList::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
{
    return FFastArraySerializer::FastArrayDeltaSerialize<FSFQuickbarEntry, FSFQuickbarList>(Entries, DeltaParams, *this);
}

void FSFQuickbarList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
    for (int32 Index : AddedIndices)
    {
        BroadcastChangedMessage(Index);
    }
}

void FSFQuickbarList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
    for (int32 Index : ChangedIndices)
    {
        BroadcastChangedMessage(Index);
    }
}

void FSFQuickbarList::BroadcastChangedMessage(int32 SlotIndex)
{
    if (QuickbarComponent && QuickbarComponent->OnQuickbarEntryChanged.IsBound())
    {
        const FSFQuickbarEntry& Entry = Entries[SlotIndex];
        QuickbarComponent->OnQuickbarEntryChanged.Broadcast(SlotIndex, Entry.ItemInstance, Entry.ItemCount);
    }
}

USFQuickbarComponent::USFQuickbarComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer), QuickbarList(this)
{
    bWantsInitializeComponent = true;
    SetIsReplicatedByDefault(true);
}

void USFQuickbarComponent::InitializeComponent()
{
    Super::InitializeComponent();

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        QuickbarList.Entries.SetNum(SlotCount);

        for (FSFQuickbarEntry& Entry : QuickbarList.Entries)
        {
            QuickbarList.MarkItemDirty(Entry);
        }
    }
}

void USFQuickbarComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, QuickbarList);
}

void USFQuickbarComponent::SaveToData(TArray<FSFSavedItemSlot>& OutSlots) const
{
    OutSlots.SetNum(QuickbarList.Entries.Num());

    for (int32 i = 0; i < QuickbarList.Entries.Num(); ++i)
    {
        const FSFQuickbarEntry& Entry = QuickbarList.Entries[i];
        USFItemInstance* Item = Entry.GetItemInstance();

        if (!Item || Entry.GetItemCount() <= 0)
        {
            continue;
        }

        FSFSavedItemSlot& Slot = OutSlots[i];
        Slot.ItemID = Item->GetItemID();
        Slot.RarityTag = Item->GetItemRarityTag();
        Slot.ItemCount = Entry.GetItemCount();

        for (const FSFGameplayTagStack& Stack : Item->GetStatContainer().GetStacks())
        {
            FSFSavedTagStack& SavedStack = Slot.StatStacks.AddDefaulted_GetRef();
            SavedStack.Tag = Stack.GetStackTag();
            SavedStack.StackCount = Stack.GetStackCount();
        }

        for (const FSFGameplayTagStack& Stack : Item->GetOwnedTagContainer().GetStacks())
        {
            FSFSavedTagStack& SavedStack = Slot.OwnedTagStacks.AddDefaulted_GetRef();
            SavedStack.Tag = Stack.GetStackTag();
            SavedStack.StackCount = Stack.GetStackCount();
        }
    }
}

void USFQuickbarComponent::RestoreFromData(const TArray<FSFSavedItemSlot>& InSlots)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    int32 RestoredCount = 0;

    for (int32 i = 0; i < InSlots.Num(); ++i)
    {
        const FSFSavedItemSlot& Slot = InSlots[i];
        if (!Slot.IsValid() || !IsValidSlotIndex(i))
        {
            continue;
        }

        USFItemInstance* NewInstance = NewObject<USFItemInstance>(GetOwner());
        NewInstance->InitializeFromSavedData(Slot);

        FSFQuickbarEntry& Entry = QuickbarList.Entries[i];
        Entry.Set(NewInstance, Slot.ItemCount);

        if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
        {
            AddReplicatedSubObject(NewInstance);
        }

        QuickbarList.MarkItemDirty(Entry);
        RestoredCount++;
    }
}

bool USFQuickbarComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
    bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

    for (FSFQuickbarEntry& Entry : QuickbarList.Entries)
    {
        if (USFItemInstance* ItemInstance = Entry.ItemInstance)
        {
            bWroteSomething |= Channel->ReplicateSubobject(ItemInstance, *Bunch, *RepFlags);
        }
    }

    return bWroteSomething;
}

void USFQuickbarComponent::ReadyForReplication()
{
    Super::ReadyForReplication();

    if (IsUsingRegisteredSubObjectList())
    {
        for (const FSFQuickbarEntry& Entry : QuickbarList.Entries)
        {
            if (USFItemInstance* ItemInstance = Entry.ItemInstance)
            {
                AddReplicatedSubObject(ItemInstance);
            }
        }
    }
}

bool USFQuickbarComponent::TryAddItem(int32 SlotIndex, USFItemInstance* ItemInstance, int32 Count)
{
    if (!IsValidSlotIndex(SlotIndex) || Count <= 0)
    {
        return false;
    }

    FSFQuickbarEntry& Entry = QuickbarList.Entries[SlotIndex];

    // 빈 슬롯에는 인스턴스 필수
    if (Entry.IsEmpty())
    {
        if (!ItemInstance)
        {
            return false;
        }
        AddItemInternal(SlotIndex, ItemInstance, Count);
        return true;
    }

    // 기존 아이템이 있으면 병합
    if (ItemInstance)
    {
        // 다른 아이템이면 실패
        if (Entry.ItemInstance->GetItemID() != ItemInstance->GetItemID() ||
            !Entry.ItemInstance->GetItemRarityTag().MatchesTagExact(ItemInstance->GetItemRarityTag()))
        {
            return false;
        }
    }

    // nullptr이거나 같은 아이템이면 카운트만 증가
    AddItemInternal(SlotIndex, nullptr, Count);
    return true;
}

bool USFQuickbarComponent::TryRemoveItem(int32 SlotIndex, int32 Count)
{
    if (!IsValidSlotIndex(SlotIndex) || IsSlotEmpty(SlotIndex))
    {
        return false;
    }

    if (GetItemCount(SlotIndex) < Count)
    {
        return false;
    }

    RemoveItemInternal(SlotIndex, Count);
    return true;
}

void USFQuickbarComponent::AddItemInternal(int32 SlotIndex, USFItemInstance* ItemInstance, int32 ItemCount)
{
    if (!IsValidSlotIndex(SlotIndex))
    {
        return;
    }

    FSFQuickbarEntry& Entry = QuickbarList.Entries[SlotIndex];

    if (Entry.ItemInstance)
    {
        Entry.AddCount(ItemCount);
    }
    else
    {
        if (ItemInstance && ItemInstance->GetOuter() != GetOwner())
        {
            ItemInstance->Rename(nullptr, GetOwner()); 
        }
        
        Entry.Set(ItemInstance, ItemCount);

        if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && ItemInstance)
        {
            AddReplicatedSubObject(ItemInstance);
        }
    }

    QuickbarList.MarkItemDirty(Entry);

    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        if (PC->IsLocalController())
        {
            QuickbarList.BroadcastChangedMessage(SlotIndex);
        }
    }
}

USFItemInstance* USFQuickbarComponent::RemoveItemInternal(int32 SlotIndex, int32 ItemCount)
{
    if (!IsValidSlotIndex(SlotIndex))
    {
        return nullptr;
    }

    FSFQuickbarEntry& Entry = QuickbarList.Entries[SlotIndex];
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

    QuickbarList.MarkItemDirty(Entry);

    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        if (PC->IsLocalController())
        {
            QuickbarList.BroadcastChangedMessage(SlotIndex);
        }
    }
    
    return ItemInstance;
}

USFItemInstance* USFQuickbarComponent::GetItemInstance(int32 SlotIndex) const
{
    if (!IsValidSlotIndex(SlotIndex))
    {
        return nullptr;
    }

    return QuickbarList.Entries[SlotIndex].ItemInstance;
}

int32 USFQuickbarComponent::GetItemCount(int32 SlotIndex) const
{
    if (!IsValidSlotIndex(SlotIndex))
    {
        return 0;
    }

    return QuickbarList.Entries[SlotIndex].ItemCount;
}

bool USFQuickbarComponent::IsSlotEmpty(int32 SlotIndex) const
{
    if (!IsValidSlotIndex(SlotIndex))
    {
        return true;
    }

    return QuickbarList.Entries[SlotIndex].IsEmpty();
}