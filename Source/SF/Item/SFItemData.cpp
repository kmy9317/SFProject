#include "SFItemData.h"

#include "Fragments/SFItemFragment_Consumable.h"
#include "UObject/ObjectSaveContext.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "SFItemInstance.h"
#include "SFItemRarityConfig.h"
#include "Fragments/SFItemFragment_Equippable.h"
#include "System/SFAssetManager.h"

const USFItemData& USFItemData::Get()
{
    return USFAssetManager::Get().GetItemData();
}

#if WITH_EDITORONLY_DATA
void USFItemData::PreSave(FObjectPreSaveContext SaveContext)
{
    Super::PreSave(SaveContext);

    // ID 정렬
    ItemIdToClass.KeySort([](int32 A, int32 B) { return A < B; });

    // 역방향 매핑 및 카테고리 캐싱 생성
    ItemClassToId.Empty();
    EquippableItemClasses.Empty();
    ConsumableItemClasses.Empty();

    for (const auto& Pair : ItemIdToClass)
    {
        if (!Pair.Value)
        {
            continue;
        }

        ItemClassToId.Add(Pair.Value, Pair.Key);

        const USFItemDefinition* Definition = Pair.Value.GetDefaultObject();
        if (!Definition)
        {
            continue;
        }

        if (Definition->FindFragment<USFItemFragment_Equippable>())
        {
            EquippableItemClasses.Add(Pair.Value);
        }

        if (Definition->FindFragment<USFItemFragment_Consumable>())
        {
            ConsumableItemClasses.Add(Pair.Value);
        }
    }

    // Rarity 정렬 (SortOrder 기준)
    Rarities.Sort([](const USFItemRarityConfig& A, const USFItemRarityConfig& B)
    {
        return A.SortOrder < B.SortOrder;
    });
}
#endif

#if WITH_EDITOR
EDataValidationResult USFItemData::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    TSet<int32> IdSet;
    TSet<TSubclassOf<USFItemDefinition>> ClassSet;

    for (const auto& Pair : ItemIdToClass)
    {
        // ID 검사
        if (Pair.Key <= 0)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Invalid ID: %d"), Pair.Key)));
            Result = EDataValidationResult::Invalid;
        }

        if (IdSet.Contains(Pair.Key))
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Duplicate ID: %d"), Pair.Key)));
            Result = EDataValidationResult::Invalid;
        }
        IdSet.Add(Pair.Key);

        // Class 검사
        if (!Pair.Value)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Null class for ID: %d"), Pair.Key)));
            Result = EDataValidationResult::Invalid;
            continue;
        }

        if (ClassSet.Contains(Pair.Value))
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Duplicate class for ID: %d"), Pair.Key)));
            Result = EDataValidationResult::Invalid;
        }
        ClassSet.Add(Pair.Value);
    }

    // Rarity 검사
    TSet<FGameplayTag> RarityTagSet;
    for (const USFItemRarityConfig* Rarity : Rarities)
    {
        if (!Rarity)
        {
            Context.AddError(FText::FromString(TEXT("Null rarity config")));
            Result = EDataValidationResult::Invalid;
            continue;
        }

        if (RarityTagSet.Contains(Rarity->RarityTag))
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Duplicate rarity tag: %s"), *Rarity->RarityTag.ToString())));
            Result = EDataValidationResult::Invalid;
        }
        RarityTagSet.Add(Rarity->RarityTag);
    }

    return Result;
}
#endif

const USFItemDefinition* USFItemData::FindDefinitionById(int32 ItemId) const
{
    if (const TSubclassOf<USFItemDefinition>* FoundClass = ItemIdToClass.Find(ItemId))
    {
        return FoundClass->GetDefaultObject();
    }
    return nullptr;
}

int32 USFItemData::FindIdByDefinition(const USFItemDefinition* Definition) const
{
    if (!Definition)
    {
        return INDEX_NONE;
    }

    if (const int32* FoundId = ItemClassToId.Find(Definition->GetClass()))
    {
        return *FoundId;
    }
    return INDEX_NONE;
}

const USFItemRarityConfig* USFItemData::FindRarityByTag(const FGameplayTag& RarityTag) const
{
    for (const USFItemRarityConfig* Rarity : Rarities)
    {
        if (Rarity && Rarity->RarityTag.MatchesTagExact(RarityTag))
        {
            return Rarity;
        }
    }
    return nullptr;
}

const USFItemRarityConfig* USFItemData::PickRandomRarity(float LuckValue) const
{
    if (Rarities.IsEmpty())
    {
        return nullptr;
    }

    float TotalWeight = 0.f;
    TArray<float> Weights;
    Weights.Reserve(Rarities.Num());

    for (const USFItemRarityConfig* Rarity : Rarities)
    {
        float Weight = Rarity ? Rarity->GetWeightForLuck(LuckValue) : 0.f;
        Weights.Add(Weight);
        TotalWeight += Weight;
    }

    if (TotalWeight <= 0.f)
    {
        return Rarities[0];
    }

    float RandomValue = FMath::FRand() * TotalWeight;
    float Accumulated = 0.f;

    for (int32 i = 0; i < Rarities.Num(); ++i)
    {
        Accumulated += Weights[i];
        if (RandomValue <= Accumulated)
        {
            return Rarities[i];
        }
    }

    return Rarities.Last();
}

USFItemInstance* USFItemData::CreateItemInstance(UObject* Outer, const USFItemDefinition* Definition, const FGameplayTag& RarityTag) const
{
    if (!Definition)
    {
        return nullptr;
    }

    // 등급 허용 체크
    if (!Definition->CanDropWithRarity(RarityTag))
    {
        return nullptr;
    }

    const int32 ItemId = FindIdByDefinition(Definition);
    if (ItemId == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateItemInstance: Definition not registered in ItemData"));
        return nullptr;
    }

    USFItemInstance* Instance = NewObject<USFItemInstance>(Outer);
    Instance->ItemID = ItemId;
    Instance->ItemRarity = RarityTag;
    
    // Fragment 초기화 (스탯 롤링 등)
    for (const USFItemFragment* Fragment : Definition->Fragments)
    {
        if (Fragment)
        {
            Fragment->OnInstanceCreated(Instance);
        }
    }
    
    return Instance;
}

USFItemInstance* USFItemData::CreateItemInstanceById(UObject* Outer, int32 ItemId, const FGameplayTag& RarityTag) const
{
    const USFItemDefinition* Definition = FindDefinitionById(ItemId);
    return CreateItemInstance(Outer, Definition, RarityTag);
}