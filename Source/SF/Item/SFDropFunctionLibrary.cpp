#include "SFDropFunctionLibrary.h"

#include "Actors/SFAutoPickup.h"
#include "Item/SFItemData.h"
#include "Item/SFItemDefinition.h"
#include "Item/SFItemInstance.h"
#include "Item/SFItemRarityConfig.h"
#include "Item/SFDropTable.h"
#include "Inventory/SFInventoryManagerComponent.h"
#include "Actors/SFPickupableItemBase.h"
#include "Fragments/SFItemFragment_AutoPickup.h"

TArray<FSFDropResult> USFDropFunctionLibrary::GenerateDropResults(UObject* Outer, const USFDropTable* DropTable, float LuckValue)
{
    TArray<FSFDropResult> Results;

    if (!Outer || !DropTable || DropTable->Entries.IsEmpty())
    {
        return Results;
    }

    const USFItemData& ItemData = USFItemData::Get();
    TArray<int32> DroppedIndices;

    for (int32 i = 0; i < DropTable->Entries.Num(); ++i)
    {
        const FSFDropTableEntry& Entry = DropTable->Entries[i];

        if (!Entry.ItemDefinitionClass)
        {
            continue;
        }

        if (DropTable->MaxDropCount > 0 && Results.Num() >= DropTable->MaxDropCount)
        {
            break;
        }

        float DropChance = Entry.GetDropChance(LuckValue);
        if (FMath::FRand() > DropChance)
        {
            continue;
        }

        FGameplayTag RarityTag = DetermineRarity(Entry, LuckValue);

        const USFItemDefinition* ItemDef = Entry.ItemDefinitionClass.GetDefaultObject();
        USFItemInstance* Instance = ItemData.CreateItemInstance(Outer, ItemDef, RarityTag);

        if (Instance)
        {
            FSFDropResult Result;
            Result.ItemInstance = Instance;
            Result.SpawnCount = Entry.RollSpawnCount();
            Result.AmountPerSpawn = Entry.RollAmountPerSpawn();
            Results.Add(Result);

            DroppedIndices.Add(i);
        }
    }

    // 보장 드롭 처리
    if (DropTable->GuaranteedDropCount > 0 && Results.Num() < DropTable->GuaranteedDropCount)
    {
        TArray<int32> RemainingIndices;
        for (int32 i = 0; i < DropTable->Entries.Num(); ++i)
        {
            if (!DroppedIndices.Contains(i) && DropTable->Entries[i].ItemDefinitionClass)
            {
                RemainingIndices.Add(i);
            }
        }

        while (Results.Num() < DropTable->GuaranteedDropCount && RemainingIndices.Num() > 0)
        {
            int32 RandomIdx = FMath::RandRange(0, RemainingIndices.Num() - 1);
            int32 EntryIdx = RemainingIndices[RandomIdx];
            RemainingIndices.RemoveAt(RandomIdx);

            const FSFDropTableEntry& Entry = DropTable->Entries[EntryIdx];
            FGameplayTag RarityTag = DetermineRarity(Entry, LuckValue);

            const USFItemDefinition* ItemDef = Entry.ItemDefinitionClass.GetDefaultObject();
            USFItemInstance* Instance = ItemData.CreateItemInstance(Outer, ItemDef, RarityTag);

            if (Instance)
            {
                FSFDropResult Result;
                Result.ItemInstance = Instance;
                Result.SpawnCount = Entry.RollSpawnCount();
                Result.AmountPerSpawn = Entry.RollAmountPerSpawn();
                Results.Add(Result);
            }
        }
    }

    return Results;
}

void USFDropFunctionLibrary::SpawnDropResults(UObject* WorldContextObject, const TArray<FSFDropResult>& DropResults, const FVector& Location, float SpawnRadius)
{
    if (!WorldContextObject)
    {
        return;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return;
    }

    const USFItemData& ItemData = USFItemData::Get();
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    for (const FSFDropResult& Result : DropResults)
    {
        if (!Result.IsValid())
        {
            continue;
        }

        const USFItemDefinition* ItemDef = ItemData.FindDefinitionById(Result.ItemInstance->GetItemID());
        if (!ItemDef)
        {
            continue;
        }

        // bSpawnIndividually: SpawnCount만큼 개별 스폰
        // !bSpawnIndividually: 1개에 총량 합산
        const bool bSpawnIndividually = ItemDef->bSpawnIndividually;
        const int32 ActualSpawnCount = bSpawnIndividually ? Result.SpawnCount : 1;
        const int32 ActualAmount = bSpawnIndividually ? Result.AmountPerSpawn : Result.GetTotalAmount();

        // 자동 습득 아이템
        if (const USFItemFragment_AutoPickup* AutoPickupFragment = ItemDef->FindFragment<USFItemFragment_AutoPickup>())
        {
            TSubclassOf<ASFAutoPickup> PickupClass = AutoPickupFragment->PickupActorClass;
            if (!PickupClass)
            {
                PickupClass = ASFAutoPickup::StaticClass();
            }

            for (int32 i = 0; i < ActualSpawnCount; ++i)
            {
                FVector2D RandPoint = FMath::RandPointInCircle(SpawnRadius);
                FVector SpawnLocation = Location + FVector(RandPoint.X, RandPoint.Y, 0.f);

                ASFAutoPickup* AutoPickup = World->SpawnActor<ASFAutoPickup>(PickupClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
                if (AutoPickup)
                {
                    AutoPickup->Initialize(Result.ItemInstance, ActualAmount);
                }
            }
            continue;
        }

        // 일반 아이템
        TSubclassOf<ASFPickupableItemBase> PickupClass = ItemData.PickupableItemClass;
        if (!PickupClass)
        {
            PickupClass = ASFPickupableItemBase::StaticClass();
        }

        for (int32 i = 0; i < ActualSpawnCount; ++i)
        {
            FVector2D RandPoint = FMath::RandPointInCircle(SpawnRadius);
            FVector SpawnLocation = Location + FVector(RandPoint.X, RandPoint.Y, 0.f);

            ASFPickupableItemBase* Pickup = World->SpawnActor<ASFPickupableItemBase>(PickupClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
            if (Pickup)
            {
                FSFPickupInfo PickupInfo;
                PickupInfo.PickupInstance.ItemInstance = Result.ItemInstance;
                PickupInfo.PickupInstance.ItemCount = ActualAmount;
                Pickup->SetPickupInfo(PickupInfo);
            }
        }
    }
}

void USFDropFunctionLibrary::ExecuteAndSpawnDrops(UObject* WorldContextObject, const USFDropTable* DropTable, float LuckValue, const FVector& Location, float SpawnRadius)
{
    TArray<FSFDropResult> Results = GenerateDropResults(WorldContextObject, DropTable, LuckValue);
    SpawnDropResults(WorldContextObject, Results, Location, SpawnRadius);
}

bool USFDropFunctionLibrary::AddDropResultsToInventory(USFInventoryManagerComponent* InventoryManager, const TArray<FSFDropResult>& DropResults)
{
    if (!InventoryManager)
    {
        return false;
    }

    bool bAllAdded = true;

    for (const FSFDropResult& Result : DropResults)
    {
        if (!Result.IsValid())
        {
            continue;
        }

        // ItemCount → GetTotalAmount()
        int32 TotalAmount = Result.GetTotalAmount();
        int32 AddedCount = InventoryManager->TryAddExistingItem(Result.ItemInstance, TotalAmount);
        if (AddedCount != TotalAmount)
        {
            bAllAdded = false;
        }
    }

    return bAllAdded;
}

FGameplayTag USFDropFunctionLibrary::DetermineRarity(const FSFDropTableEntry& Entry, float LuckValue)
{
    // 고정 등급이 있으면 사용
    if (Entry.FixedRarityTag.IsValid())
    {
        return Entry.FixedRarityTag;
    }

    // Luck 기반 랜덤
    const USFItemRarityConfig* RarityConfig = USFItemData::Get().PickRandomRarity(LuckValue);
    if (RarityConfig)
    {
        return RarityConfig->RarityTag;
    }

    return FGameplayTag();
}