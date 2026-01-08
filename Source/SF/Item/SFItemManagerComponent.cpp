#include "SFItemManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "SFItemGameplayTags.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Item/SFItemData.h"
#include "Item/SFItemDefinition.h"
#include "Item/SFItemInstance.h"
#include "Item/Fragments/SFItemFragment_Consumable.h"
#include "Inventory/SFInventoryManagerComponent.h"
#include "Actors/SFPickupableItemBase.h"
#include "Inventory/SFQuickbarComponent.h"

USFItemManagerComponent::USFItemManagerComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsReplicatedByDefault(true);
}

void USFItemManagerComponent::Server_MoveItem_Implementation(FSFItemSlotHandle FromSlot, FSFItemSlotHandle ToSlot)
{
    if (!HasAuthority())
    {
        return;
    }

    // 검증
    if (!IsValidSlot(FromSlot) || !IsValidSlot(ToSlot))
    {
        return;
    }

    // 같은 슬롯이면 무시
    if (FromSlot.SlotType == ToSlot.SlotType && FromSlot.SlotIndex == ToSlot.SlotIndex)
    {
        return;
    }

    // From이 비어있으면 무시
    if (IsSlotEmpty(FromSlot))
    {
        return;
    }

    USFItemInstance* FromInstance = nullptr;
    int32 FromCount = 0;
    GetSlotInfo(FromSlot, FromInstance, FromCount);

    if (!FromInstance || FromCount <= 0)
    {
        return;
    }

    // ========== To가 비어있는 경우: 이동 ==========
    if (IsSlotEmpty(ToSlot))
    {
        RemoveFromSlot(FromSlot, FromCount);
        AddToSlot(ToSlot, FromInstance, FromCount);
        return;
    }

    // ========== To에 아이템이 있는 경우 ==========

    USFItemInstance* ToInstance = nullptr;
    int32 ToCount = 0;
    GetSlotInfo(ToSlot, ToInstance, ToCount);

    bool bSameItem = (ToInstance->GetItemID() == FromInstance->GetItemID() && ToInstance->GetItemRarityTag().MatchesTagExact(FromInstance->GetItemRarityTag()));
    if (bSameItem)
    {
        // 같은 아이템: 병합 시도
        const USFItemDefinition* ItemDef = USFItemData::Get().FindDefinitionById(FromInstance->GetItemID());
        if (ItemDef && ItemDef->MaxStackCount > 1)
        {
            int32 Space = ItemDef->MaxStackCount - ToCount;
            if (Space > 0)
            {
                // 병합 가능
                int32 ToMerge = FMath::Min(Space, FromCount);
                RemoveFromSlot(FromSlot, ToMerge);
                AddToSlot(ToSlot, nullptr, ToMerge);
                return;
            }
            // Space <= 0: 병합 불가능 → 스왑으로 진행 (아래로 fall-through)
        }
        else
        {
            // MaxStackCount == 1: 스왑으로 진행
        }
    }

    // ========== 스왑 (다른 아이템 또는 병합 불가능한 같은 아이템) ==========

    RemoveFromSlot(FromSlot, FromCount);
    RemoveFromSlot(ToSlot, ToCount);

    AddToSlot(FromSlot, ToInstance, ToCount);
    AddToSlot(ToSlot, FromInstance, FromCount);
}

void USFItemManagerComponent::Server_UseItem_Implementation(FSFItemSlotHandle Slot)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!IsValidSlot(Slot) || IsSlotEmpty(Slot))
    {
        return;
    }

    USFItemInstance* ItemInstance = nullptr;
    int32 ItemCount = 0;
    if (!GetSlotInfo(Slot, ItemInstance, ItemCount))
    {
        return;
    }

    // ========== Fragment 타입별 분기 ==========

    if (const USFItemFragment_Consumable* ConsumeFrag = ItemInstance->FindFragmentByClass<USFItemFragment_Consumable>())
    {
        UseConsumableItem(ItemInstance, ConsumeFrag, Slot);
        return;
    }

    // 추후 다른 타입 추가
    }

void USFItemManagerComponent::UseConsumableItem(USFItemInstance* ItemInstance, const class USFItemFragment_Consumable* ConsumeFrag, const FSFItemSlotHandle& Slot)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    FGameplayTag TriggerTag = GetConsumeAbilityTriggerTag(ConsumeFrag);
    if (!TriggerTag.IsValid())
    {
        return;
    }

    FGameplayEventData EventData;
    EventData.OptionalObject = ItemInstance;
    EventData.Instigator = GetOwner();
    EventData.EventMagnitude = static_cast<float>(Slot.SlotIndex);

    ASC->HandleGameplayEvent(TriggerTag, &EventData);
}

void USFItemManagerComponent::Server_DropItem_Implementation(FSFItemSlotHandle Slot)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!IsValidSlot(Slot) || IsSlotEmpty(Slot))
    {
        return;
    }

    USFItemInstance* ItemInstance = nullptr;
    int32 ItemCount = 0;
    if (!GetSlotInfo(Slot, ItemInstance, ItemCount))
    {
        return;
    }

    if (SpawnDroppedItem(ItemInstance, ItemCount))
    {
        RemoveFromSlot(Slot, ItemCount);
    }
}

bool USFItemManagerComponent::TryPickupItem(ASFPickupableItemBase* PickupableItem)
{
    if (!HasAuthority())
    {
        return false;
    }

    ISFPickupable* Pickupable = Cast<ISFPickupable>(PickupableItem);
    if (!Pickupable)
    {
        return false;
    }

    USFInventoryManagerComponent* InventoryManager = GetInventoryManager();
    if (!InventoryManager)
    {
        return false;
    }

    const FSFPickupInfo PickupInfo = Pickupable->GetPickupInfo();

    int32 ItemID = 0;
    FGameplayTag RarityTag;
    int32 ItemCount = 0;
    USFItemInstance* ExistingInstance = nullptr;

    if (PickupInfo.PickupInstance.ItemInstance)
    {
        ExistingInstance = PickupInfo.PickupInstance.ItemInstance;
        ItemID = ExistingInstance->GetItemID();
        RarityTag = ExistingInstance->GetItemRarityTag();
        ItemCount = PickupInfo.PickupInstance.ItemCount;
    }
    else if (PickupInfo.PickupDefinition.ItemDefinitionClass)
    {
        const USFItemDefinition* ItemDef = PickupInfo.PickupDefinition.ItemDefinitionClass.GetDefaultObject();
        ItemID = USFItemData::Get().FindIdByDefinition(ItemDef);
        RarityTag = PickupInfo.PickupDefinition.RarityTag;
        ItemCount = PickupInfo.PickupDefinition.ItemCount;
    }

    if (ItemID <= 0 || !RarityTag.IsValid() || ItemCount <= 0)
    {
        return false;
    }

    TArray<int32> SlotIndices;
    TArray<int32> Counts;

    int32 AddableCount = InventoryManager->CanAddItem(ItemID, RarityTag, ItemCount, SlotIndices, Counts);

    if (AddableCount != ItemCount)
    {
        return false;
    }

    const USFItemData& ItemData = USFItemData::Get();
    const USFItemDefinition* ItemDef = ItemData.FindDefinitionById(ItemID);

    for (int32 i = 0; i < SlotIndices.Num(); ++i)
    {
        const int32 SlotIndex = SlotIndices[i];
        const int32 Count = Counts[i];

        if (InventoryManager->IsSlotEmpty(SlotIndex))
        {
            USFItemInstance* InstanceToAdd = nullptr;

            if (ExistingInstance)
            {
                InstanceToAdd = ExistingInstance;
                ExistingInstance = nullptr;
            }
            else
            {
                InstanceToAdd = ItemData.CreateItemInstance(GetOwner(), ItemDef, RarityTag);
            }

            InventoryManager->AddItemInternal(SlotIndex, InstanceToAdd, Count);
        }
        else
        {
            InventoryManager->AddItemInternal(SlotIndex, nullptr, Count);
        }
    }

    PickupableItem->Destroy();
    return true;
}

void USFItemManagerComponent::ConsumeFromSlot(const FSFItemSlotHandle& Slot, int32 Count)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!IsValidSlot(Slot) || IsSlotEmpty(Slot))
    {
        return;
    }

    RemoveFromSlot(Slot, Count);
}

bool USFItemManagerComponent::IsValidSlot(const FSFItemSlotHandle& Slot) const
{
    if (!Slot.IsValid())
    {
        return false;
    }

    switch (Slot.SlotType)
    {
    case ESFItemSlotType::Inventory:
        if (USFInventoryManagerComponent* Inv = GetInventoryManager())
        {
            return Inv->IsValidSlotIndex(Slot.SlotIndex);
        }
        break;
    case ESFItemSlotType::Quickbar:
        if (USFQuickbarComponent* QB = GetQuickbarComponent())
        {
            return QB->IsValidSlotIndex(Slot.SlotIndex);
        }
        break;
    }
    return false;
}

bool USFItemManagerComponent::IsSlotEmpty(const FSFItemSlotHandle& Slot) const
{
    switch (Slot.SlotType)
    {
    case ESFItemSlotType::Inventory:
        if (USFInventoryManagerComponent* Inv = GetInventoryManager())
        {
            return Inv->IsSlotEmpty(Slot.SlotIndex);
        }
        break;
    case ESFItemSlotType::Quickbar:
        if (USFQuickbarComponent* QB = GetQuickbarComponent())
        {
            return QB->IsSlotEmpty(Slot.SlotIndex);
        }
        break;
    }
    return true;
}

bool USFItemManagerComponent::GetSlotInfo(const FSFItemSlotHandle& Slot, USFItemInstance*& OutInstance, int32& OutCount) const
{
    OutInstance = nullptr;
    OutCount = 0;

    switch (Slot.SlotType)
    {
    case ESFItemSlotType::Inventory:
        if (USFInventoryManagerComponent* Inv = GetInventoryManager())
        {
            OutInstance = Inv->GetItemInstance(Slot.SlotIndex);
            OutCount = Inv->GetItemCount(Slot.SlotIndex);
            return OutInstance != nullptr;
        }
        break;
    case ESFItemSlotType::Quickbar:
        if (USFQuickbarComponent* QB = GetQuickbarComponent())
        {
            OutInstance = QB->GetItemInstance(Slot.SlotIndex);
            OutCount = QB->GetItemCount(Slot.SlotIndex);
            return OutInstance != nullptr;
        }
        break;
    }
    return false;
}

void USFItemManagerComponent::RemoveFromSlot(const FSFItemSlotHandle& Slot, int32 Count)
{
    switch (Slot.SlotType)
    {
    case ESFItemSlotType::Inventory:
        if (USFInventoryManagerComponent* Inv = GetInventoryManager())
        {
            Inv->RemoveItemInternal(Slot.SlotIndex, Count);
        }
        break;
    case ESFItemSlotType::Quickbar:
        if (USFQuickbarComponent* QB = GetQuickbarComponent())
        {
            QB->RemoveItemInternal(Slot.SlotIndex, Count);
        }
        break;
    }
}

void USFItemManagerComponent::AddToSlot(const FSFItemSlotHandle& Slot, USFItemInstance* Instance, int32 Count)
{
    switch (Slot.SlotType)
    {
    case ESFItemSlotType::Inventory:
        if (USFInventoryManagerComponent* Inv = GetInventoryManager())
        {
            Inv->AddItemInternal(Slot.SlotIndex, Instance, Count);
        }
        break;
    case ESFItemSlotType::Quickbar:
        if (USFQuickbarComponent* QB = GetQuickbarComponent())
        {
            QB->AddItemInternal(Slot.SlotIndex, Instance, Count);
        }
        break;
    }
}

USFInventoryManagerComponent* USFItemManagerComponent::GetInventoryManager() const
{
    if (AController* Controller = Cast<AController>(GetOwner()))
    {
        return Controller->FindComponentByClass<USFInventoryManagerComponent>();
    }
    return nullptr;
}

USFQuickbarComponent* USFItemManagerComponent::GetQuickbarComponent() const
{
    if (AController* Controller = Cast<AController>(GetOwner()))
    {
        return Controller->FindComponentByClass<USFQuickbarComponent>();
    }
    return nullptr;
}

UAbilitySystemComponent* USFItemManagerComponent::GetAbilitySystemComponent() const
{
    AController* Controller = Cast<AController>(GetOwner());
    if (!Controller)
    {
        return nullptr;
    }

    if (APlayerState* PS = Controller->GetPlayerState<APlayerState>())
    {
        return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS);
    }

    return nullptr;
}

bool USFItemManagerComponent::SpawnDroppedItem(USFItemInstance* ItemInstance, int32 ItemCount)
{
    if (!ItemInstance || ItemCount <= 0)
    {
        return false;
    }

    const USFItemData& ItemData = USFItemData::Get();
    TSubclassOf<ASFPickupableItemBase> PickupClass = ItemData.PickupableItemClass;

    if (!PickupClass)
    {
        PickupClass = ASFPickupableItemBase::StaticClass();
    }

    if (!PickupClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("PickupableItemClass is not set!"));
        return false;
    }

    AController* Controller = Cast<AController>(GetOwner());
    ACharacter* Character = Controller ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
    if (!Character)
    {
        return false;
    }

    const float MaxDistance = 100.f;
    const int32 MaxTryCount = 5;
    float HalfRadius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius() / 2.f;
    float QuarterHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() / 2.f;
    TArray<AActor*> ActorsToIgnore = { Character };

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

    for (int32 i = 0; i < MaxTryCount; ++i)
    {
        FHitResult HitResult;
        FVector2D RandPoint = FMath::RandPointInCircle(MaxDistance);
        FVector TraceStart = Character->GetCapsuleComponent()->GetComponentLocation();
        FVector TraceEnd = TraceStart + FVector(RandPoint.X, RandPoint.Y, 0.f);

        bool bHit = UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), TraceStart, TraceEnd, HalfRadius, QuarterHeight, UEngineTypes::ConvertToTraceType(ECC_Visibility), false, ActorsToIgnore, EDrawDebugTrace::None, HitResult, true);

        if (bHit)
        {
            continue;
        }

        ASFPickupableItemBase* DroppedItem = GetWorld()->SpawnActor<ASFPickupableItemBase>(PickupClass, TraceEnd, FRotator::ZeroRotator, SpawnParams);

        if (DroppedItem)
        {
            FSFPickupInfo PickupInfo;
            PickupInfo.PickupInstance.ItemInstance = ItemInstance;
            PickupInfo.PickupInstance.ItemCount = ItemCount;

            DroppedItem->SetPickupInfo(PickupInfo);
            return true;
        }
    }

    return false;
}

FGameplayTag USFItemManagerComponent::GetConsumeAbilityTriggerTag(const USFItemFragment_Consumable* ConsumeFrag) const
{
    if (!ConsumeFrag || !ConsumeFrag->ConsumeTypeTag.IsValid())
    {
        return FGameplayTag();
    }

    if (ConsumeFrag->ConsumeTypeTag.MatchesTag(SFGameplayTags::Item_Consumable_Potion))
    {
        return SFGameplayTags::Ability_Hero_Drink;
    }

    return FGameplayTag();
}