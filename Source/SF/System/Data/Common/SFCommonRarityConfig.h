#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SFCommonRarityConfig.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFCommonRarityConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(GetCommonRarityConfigAssetType(), GetFName());
	}
	static FPrimaryAssetType GetCommonRarityConfigAssetType() { return FPrimaryAssetType(TEXT("CommonRarityConfig")); }

	float GetWeightForLuck(float LuckValue) const;
	
public:

	// 희귀도 식별 태그 (ex: Rarity.Epic)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Identity")
	FGameplayTag RarityTag;

	// 기본 등장 확률 가중치 (Luck 0일 때)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Weight")
	float BaseWeight = 0.f;

	/** 
	 * 운(Luck) 수치에 따른 가중치 추가 보정값
	 * X축: Luck 수치
	 * Y축: 가중치에 더할 값 (예: +5, -30)
	 * 예시: Epic 등급은 Luck이 높을수록 Y값이 올라가는 커브 설정
	 * 예시: Uncommon 등급은 Luck이 높을수록 Y값이 내려가는 커브 설정
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Weight", meta = (AssetBundles = "InGame"))
	TSoftObjectPtr<UCurveFloat> LuckWeightCurve;

	// 등급별 프레임/배경 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|UI", meta = (AssetBundles = "InGame"))
	TSoftObjectPtr<UTexture2D> FrameTexture;

	// 등급별 색상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|UI")
	FLinearColor FrameColor = FLinearColor::White;
};
