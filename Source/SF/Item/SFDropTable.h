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

	UPROPERTY(BlueprintReadOnly, Category = "Drop")
	int32 ItemCount = 0;

	bool IsValid() const { return ItemInstance != nullptr && ItemCount > 0; }
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

	// 수량 범위
	UPROPERTY(EditDefaultsOnly, Category = "Drop", meta = (ClampMin = "1"))
	int32 MinCount = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Drop", meta = (ClampMin = "1"))
	int32 MaxCount = 1;

	// 등급 고정 (Invalid면 Luck 기반 랜덤)
	UPROPERTY(EditDefaultsOnly, Category = "Drop", meta = (Categories = "Rarity"))
	FGameplayTag FixedRarityTag;

	// 최종 드롭 확률 계산
	float GetDropChance(float LuckValue) const;

	// 수량 롤링
	int32 RollCount() const;
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
