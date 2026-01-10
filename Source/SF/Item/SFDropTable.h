#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SFDropTable.generated.h"

class USFItemDefinition;
class USFItemInstance;

USTRUCT(BlueprintType)
struct FSFDropResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Drop")
	TObjectPtr<USFItemInstance> ItemInstance;

	// 스폰할 액터 수
	UPROPERTY(BlueprintReadOnly, Category = "Drop")
	int32 SpawnCount = 1;

	// 액터당 양
	UPROPERTY(BlueprintReadOnly, Category = "Drop")
	int32 AmountPerSpawn = 1;
	
	bool IsValid() const { return ItemInstance != nullptr && SpawnCount > 0 && AmountPerSpawn > 0; }

	// 총량
	int32 GetTotalAmount() const { return SpawnCount * AmountPerSpawn; }
};

// 드롭 테이블 항목
USTRUCT(BlueprintType)
struct FSFDropTableEntry
{
	GENERATED_BODY()

	// 드롭할 아이템
	UPROPERTY(EditDefaultsOnly, Category = "Drop")
	TSubclassOf<USFItemDefinition> ItemDefinitionClass;

	// 기본 드롭 확률 (0.0 ~ 1.0)
	UPROPERTY(EditDefaultsOnly, Category = "Drop", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseDropChance = 1.f;

	// Luck에 따른 드롭 확률 보정 커브
	// X축: Luck (0~100), Y축: 배율 (0.5 ~ 2.0 등)
	UPROPERTY(EditDefaultsOnly, Category = "Drop")
	TObjectPtr<UCurveFloat> LuckDropChanceCurve;

	// 스폰할 최소 액터 수 (bSpawnIndividually일 때만 사용)
	UPROPERTY(EditDefaultsOnly, Category = "Drop", meta = (ClampMin = "1"))
	int32 MinSpawnCount = 1;

	// 스폰할 최대 액터 수 (bSpawnIndividually일 때만 사용)
	UPROPERTY(EditDefaultsOnly, Category = "Drop", meta = (ClampMin = "1"))
	int32 MaxSpawnCount = 1;

	// 액터당 양 (골드량, 스택 수 등)
	UPROPERTY(EditDefaultsOnly, Category = "Drop", meta = (ClampMin = "1"))
	int32 MinAmountPerSpawn = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Drop", meta = (ClampMin = "1"))
	int32 MaxAmountPerSpawn = 1;

	// 등급 고정 (Invalid면 Luck 기반 랜덤)
	UPROPERTY(EditDefaultsOnly, Category = "Drop", meta = (Categories = "Rarity"))
	FGameplayTag FixedRarityTag;

	// 최종 드롭 확률 계산
	float GetDropChance(float LuckValue) const;
	int32 RollSpawnCount() const;
	int32 RollAmountPerSpawn() const;
};

/**
 * 
 */
UCLASS()
class SF_API USFDropTable : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
	
    // 드롭 항목들
    UPROPERTY(EditDefaultsOnly, Category = "Drop")
    TArray<FSFDropTableEntry> Entries;

    // 보장 드롭 수 (최소 N개는 반드시 드롭)
    UPROPERTY(EditDefaultsOnly, Category = "Drop", meta = (ClampMin = "0"))
    int32 GuaranteedDropCount = 0;

    // 최대 드롭 수
    UPROPERTY(EditDefaultsOnly, Category = "Drop", meta = (ClampMin = "0"))
    int32 MaxDropCount = 1;
};
