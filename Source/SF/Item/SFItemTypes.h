#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFItemTypes.generated.h"

// 등급별 수치 범위
USTRUCT(BlueprintType)
struct FSFRarityValueRange
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, meta = (Categories = "Rarity"))
	FGameplayTag RarityTag;

	UPROPERTY(EditDefaultsOnly)
	float MinValue = 0.f;

	UPROPERTY(EditDefaultsOnly)
	float MaxValue = 0.f;

	float RollValue() const;
	bool IsFixedValue() const;
};

// 태그 + 등급별 수치 세트
USTRUCT(BlueprintType)
struct FSFTaggedRarityValues
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, meta = (Categories = "Stat,Data"))
	FGameplayTag ValueTag;

	UPROPERTY(EditDefaultsOnly)
	TArray<FSFRarityValueRange> RarityValues;

	bool GetRangeForRarity(const FGameplayTag& RarityTag, float& OutMin, float& OutMax) const;
	float RollValueForRarity(const FGameplayTag& RarityTag) const;
	float GetFixedValueForRarity(const FGameplayTag& RarityTag) const;
};