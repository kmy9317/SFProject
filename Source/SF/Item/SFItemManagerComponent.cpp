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

USFItemManagerComponent::USFItemManagerComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsReplicatedByDefault(true);
}

void USFItemManagerComponent::Server_MoveItem_Implementation(int32 FromSlotIndex, int32 ToSlotIndex)
{
    if (!HasAuthority())
    {
        return;
    }

    USFInventoryManagerComponent* InventoryManager = GetInventoryManager();
    if (!InventoryManager)
    {
        return;
    }

    // 같은 위치면 무시
    if (FromSlotIndex == ToSlotIndex)
    {
        return;
    }

    if (!InventoryManager->IsValidSlotIndex(FromSlotIndex) || !InventoryManager->IsValidSlotIndex(ToSlotIndex))
    {
        return;
    }

    // From 슬롯이 비어있으면 무시
    if (InventoryManager->IsSlotEmpty(FromSlotIndex))
    {
        return;
    }

    USFItemInstance* FromInstance = InventoryManager->GetItemInstance(FromSlotIndex);
    int32 FromCount = InventoryManager->GetItemCount(FromSlotIndex);

    if (InventoryManager->IsSlotEmpty(ToSlotIndex))
    {
        // To가 비어있으면 단순 이동
        InventoryManager->RemoveItemInternal(FromSlotIndex, FromCount);
        InventoryManager->AddItemInternal(ToSlotIndex, FromInstance, FromCount);
    }
    else
    {
        // To에 아이템이 있으면 병합 또는 스왑
        USFItemInstance* ToInstance = InventoryManager->GetItemInstance(ToSlotIndex);
        int32 ToCount = InventoryManager->GetItemCount(ToSlotIndex);

        // 같은 아이템 + 같은 등급이면 병합 시도
        if (FromInstance->GetItemID() == ToInstance->GetItemID() && FromInstance->GetItemRarityTag().MatchesTagExact(ToInstance->GetItemRarityTag()))
        {
            const USFItemDefinition* ItemDef = USFItemData::Get().FindDefinitionById(FromInstance->GetItemID());
            if (ItemDef && ItemDef->MaxStackCount > 1)
            {
                int32 Space = ItemDef->MaxStackCount - ToCount;
                int32 ToMerge = FMath::Min(Space, FromCount);

                if (ToMerge > 0)
                {
                    InventoryManager->RemoveItemInternal(FromSlotIndex, ToMerge);
                    InventoryManager->AddItemInternal(ToSlotIndex, nullptr, ToMerge);
                }
                return;
            }
        }

        // 병합 불가능하면 스왑
        InventoryManager->RemoveItemInternal(FromSlotIndex, FromCount);
        InventoryManager->RemoveItemInternal(ToSlotIndex, ToCount);
        InventoryManager->AddItemInternal(FromSlotIndex, ToInstance, ToCount);
        InventoryManager->AddItemInternal(ToSlotIndex, FromInstance, FromCount);
    }
}

void USFItemManagerComponent::Server_UseItem_Implementation(int32 SlotIndex)
{
    if (!HasAuthority())
    {
        return;
    }

    USFInventoryManagerComponent* InventoryManager = GetInventoryManager();
    if (!InventoryManager)
    {
        return;
    }

    if (!InventoryManager->IsValidSlotIndex(SlotIndex) || InventoryManager->IsSlotEmpty(SlotIndex))
    {
        return;
    }

    USFItemInstance* ItemInstance = InventoryManager->GetItemInstance(SlotIndex);
    if (!ItemInstance)
    {
        return;
    }

    // Consumable Fragment 확인
    const USFItemFragment_Consumable* ConsumeFrag = ItemInstance->FindFragmentByClass<USFItemFragment_Consumable>();
    if (!ConsumeFrag)
    {
        return;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    // 어빌리티 트리거 태그 결정
    FGameplayTag TriggerTag = GetAbilityTriggerTag(ConsumeFrag);
    if (!TriggerTag.IsValid())
    {
        return;
    }

    // GameplayEvent로 어빌리티 활성화
    FGameplayEventData EventData;
    EventData.OptionalObject = ItemInstance;
    EventData.Instigator = GetOwner();
    EventData.EventMagnitude = static_cast<float>(SlotIndex);  // 슬롯 인덱스 전달

    ASC->HandleGameplayEvent(TriggerTag, &EventData);

    // 아이템 소모는 어빌리티 내에서 성공 시 처리
    // 어빌리티에서 InventoryManager->TryRemoveItemAt(SlotIndex, 1) 호출
}

void USFItemManagerComponent::Server_DropItem_Implementation(int32 SlotIndex, int32 DropCount)
{
    if (!HasAuthority())
    {
        return;
    }

    USFInventoryManagerComponent* InventoryManager = GetInventoryManager();
    if (!InventoryManager)
    {
        return;
    }

    if (!InventoryManager->IsValidSlotIndex(SlotIndex) || InventoryManager->IsSlotEmpty(SlotIndex))
    {
        return;
    }

    USFItemInstance* ItemInstance = InventoryManager->GetItemInstance(SlotIndex);
    int32 CurrentCount = InventoryManager->GetItemCount(SlotIndex);

    if (!ItemInstance || CurrentCount <= 0 || DropCount <= 0)
    {
        return;
    }

    // 드롭할 수량 조정
    int32 ActualDropCount = FMath::Min(DropCount, CurrentCount);

    // 드롭 아이템 스폰
    if (SpawnDroppedItem(ItemInstance, ActualDropCount))
    {
        InventoryManager->TryRemoveItemAt(SlotIndex, ActualDropCount);
    }
}

bool USFItemManagerComponent::TryPickupItem(ASFPickupableItemBase* PickupableItem)
{
    if (!HasAuthority())
    {
        return false;
    }

    // ISFPickupable 인터페이스 확인
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

    // Instance 우선, 없으면 Definition 사용
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

    // 인벤토리에 추가 가능한지 확인
    TArray<int32> SlotIndices;
    TArray<int32> Counts;

    int32 AddableCount = InventoryManager->CanAddItem(ItemID, RarityTag, ItemCount, SlotIndices, Counts);

    // 전부 추가 가능할 때만 픽업
    if (AddableCount != ItemCount)
    {
        return false;
    }

    // 인스턴스가 없으면 새로 생성
    if (!ExistingInstance)
    {
        const USFItemDefinition* ItemDef = USFItemData::Get().FindDefinitionById(ItemID);
        ExistingInstance = USFItemData::Get().CreateItemInstance(PickupableItem, ItemDef, RarityTag);
    }

    // 인벤토리에 추가
    for (int32 i = 0; i < SlotIndices.Num(); ++i)
    {
        InventoryManager->AddItemInternal(SlotIndices[i], ExistingInstance, Counts[i]);
    }

    // 픽업 액터 제거
    PickupableItem->Destroy();

    return true;
}

USFInventoryManagerComponent* USFItemManagerComponent::GetInventoryManager() const
{
    if (AController* Controller = Cast<AController>(GetOwner()))
    {
        return Controller->FindComponentByClass<USFInventoryManagerComponent>();
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
        PickupClass = ASFPickupableItemBase::StaticClass();  // 폴백
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

        bool bHit = UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), TraceStart, TraceEnd, HalfRadius, QuarterHeight, UEngineTypes::ConvertToTraceType(ECC_Visibility), false, ActorsToIgnore, EDrawDebugTrace::None,HitResult, true);

        if (bHit)
        {
            continue;
        }

        ASFPickupableItemBase* DroppedItem = GetWorld()->SpawnActor<ASFPickupableItemBase>(PickupClass, TraceEnd, FRotator::ZeroRotator, SpawnParams);

        if (DroppedItem)
        {
            // 기존 FSFPickupInfo 구조 사용
            FSFPickupInfo PickupInfo;
            PickupInfo.PickupInstance.ItemInstance = ItemInstance;
            PickupInfo.PickupInstance.ItemCount = ItemCount;
            
            DroppedItem->SetPickupInfo(PickupInfo);
            return true;
        }
    }

    return false;
}

FGameplayTag USFItemManagerComponent::GetAbilityTriggerTag(const USFItemFragment_Consumable* ConsumeFrag) const
{
    if (!ConsumeFrag || !ConsumeFrag->ConsumeTypeTag.IsValid())
    {
        return FGameplayTag();
    }

    // ConsumeTypeTag → AbilityTriggerTag 매핑
    if (ConsumeFrag->ConsumeTypeTag.MatchesTag(SFGameplayTags::Item_Consumable_Potion))
    {
        return SFGameplayTags::Ability_Hero_Drink;
    }

    // 필요시 다른 타입 추가
    // if (ConsumeFrag->ConsumeTypeTag.MatchesTag(SFGameplayTags::Consumable_Food))
    // {
    //     return SFGameplayTags::Ability_Hero_Eat;
    // }

    return FGameplayTag();
}

