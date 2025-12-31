#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SFCommonUpgradeDefinition.generated.h"

class USFCommonUpgradeFragment;

/**
 * 어떤 업그레이드 인지 정의하는 메인 데이터 에셋
 */
UCLASS()
class SF_API USFCommonUpgradeDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(GetCommonUpgradeDefinitionAssetType(), GetFName());
	}
	static FPrimaryAssetType GetCommonUpgradeDefinitionAssetType() { return FPrimaryAssetType(TEXT("CommonUpgradeDefinition")); }

	bool IsAllowedForRarity(const FGameplayTag& RarityTag) const;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|UI")
	FText DisplayName;

	// 예: "공격력이 {0} 증가합니다."
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|UI", meta = (MultiLine = "true"))
	FText DescriptionFormat;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|UI", meta = (AssetBundles = "InGame"))
	TSoftObjectPtr<UTexture2D> Icon;

	// 허용된 등급 태그 (비어있으면 모든 등급에서 출현) 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Restriction")
	FGameplayTagContainer AllowedRarityTags;
	
	// 조립식 기능 정의 (Fragments)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Behavior")
	TArray<TObjectPtr<USFCommonUpgradeFragment>> Fragments;

	template <typename ResultClass>
	const ResultClass* FindFragment() const
	{
		for (const USFCommonUpgradeFragment* Fragment : Fragments)
		{
			if (const ResultClass* Result = Cast<ResultClass>(Fragment))
			{
				return Result;
			}
		}
		return nullptr;
	}
};
