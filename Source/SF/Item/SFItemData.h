#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SFItemData.generated.h"

class ASFPickupableItemBase;
struct FGameplayTag;
class USFItemDefinition;
class USFItemRarityConfig;
class USFItemInstance;

/**
 * 
 */
UCLASS()
class SF_API USFItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
    static const USFItemData& Get();

public:
#if WITH_EDITORONLY_DATA
    virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#endif

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

    // ========== Definition 조회 ==========
    UFUNCTION(BlueprintPure, Category = "Item")
    const USFItemDefinition* FindDefinitionById(int32 ItemId) const;

    UFUNCTION(BlueprintPure, Category = "Item")
    int32 FindIdByDefinition(const USFItemDefinition* Definition) const;

    // ========== Rarity 조회 ==========
    UFUNCTION(BlueprintPure, Category = "Item")
    const USFItemRarityConfig* FindRarityByTag(const FGameplayTag& RarityTag) const;

    UFUNCTION(BlueprintPure, Category = "Item")
    const USFItemRarityConfig* PickRandomRarity(float LuckValue) const;

    UFUNCTION(BlueprintPure, Category = "Item")
    TArray<USFItemRarityConfig*> GetAllRarities() const { return Rarities; }

    // ========== Instance 생성 ==========
    UFUNCTION(BlueprintCallable, Category = "Item")
    USFItemInstance* CreateItemInstance(UObject* Outer, const USFItemDefinition* Definition, const FGameplayTag& RarityTag) const;

    UFUNCTION(BlueprintCallable, Category = "Item")
    USFItemInstance* CreateItemInstanceById(UObject* Outer,int32 ItemId, const FGameplayTag& RarityTag) const;

    // ========== 카테고리별 조회 ==========
    const TArray<TSubclassOf<USFItemDefinition>>& GetEquippableItems() const { return EquippableItemClasses; }
    const TArray<TSubclassOf<USFItemDefinition>>& GetConsumableItems() const { return ConsumableItemClasses; }

public:
    UPROPERTY(EditDefaultsOnly, Category = "Drop")
    TSubclassOf<ASFPickupableItemBase> PickupableItemClass;

private:
    // ========== Item ID 매핑 ==========
    UPROPERTY(EditDefaultsOnly, Category = "Items")
    TMap<int32, TSubclassOf<USFItemDefinition>> ItemIdToClass;

    // 역방향 매핑 (PreSave에서 생성)
    UPROPERTY()
    TMap<TSubclassOf<USFItemDefinition>, int32> ItemClassToId;

    // ========== Rarity 설정 ==========
    UPROPERTY(EditDefaultsOnly, Category = "Rarity")
    TArray<USFItemRarityConfig*> Rarities;

    // ========== 카테고리 캐싱 (PreSave에서 생성) ==========
    UPROPERTY()
    TArray<TSubclassOf<USFItemDefinition>> EquippableItemClasses;

    UPROPERTY()
    TArray<TSubclassOf<USFItemDefinition>> ConsumableItemClasses;
    
};
