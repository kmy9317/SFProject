// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SFItemRarityConfig.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFItemRarityConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// Luck에 따른 보정된 가중치 반환
	float GetWeightForLuck(float LuckValue) const;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Identity", meta = (Categories = "Rarity"))
	FGameplayTag RarityTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|UI")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|UI")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|UI")
	FSlateBrush BackgroundBrush;

	// 정렬 순서 (높을수록 좋은 등급)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Rarity")
	int32 SortOrder = 0;

	// 기본 드랍 가중치
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|DropRate")
	float BaseDropWeight = 100.f;

	// Luck에 따른 가중치 커브 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DropRate")
	TObjectPtr<UCurveFloat> LuckWeightCurve;
};
