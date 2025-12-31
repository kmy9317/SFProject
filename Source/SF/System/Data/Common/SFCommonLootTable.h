#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SFCommonLootTable.generated.h"

class USFCommonLootTable;
class USFCommonUpgradeDefinition;

/**
 * 루트 테이블 항목
 */
USTRUCT(BlueprintType)
struct FSFCommonLootEntry
{
	GENERATED_BODY()

	// 등장 확률 가중치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	float Weight = 1.0f;

	// 업그레이드 아이템
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TSoftObjectPtr<USFCommonUpgradeDefinition> UpgradeDefinition;
};

/**
 * 일반 강화 선택지 풀을 관리하는 데이터 에셋
 */
UCLASS()
class SF_API USFCommonLootTable : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(GetCommonLootTableAssetType(), GetFName());
	}
	static FPrimaryAssetType GetCommonLootTableAssetType() { return FPrimaryAssetType(TEXT("CommonLootTable")); }

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override; 
#endif

	// 이 테이블에 포함된 항목들
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TArray<FSFCommonLootEntry> LootEntries;

	float GetCachedTotalWeight() const { return CachedTotalWeight; }

private:
	void RecalculateTotalWeight();
	
private:
	UPROPERTY(Transient)
	float CachedTotalWeight = 0.0f;
	
};
