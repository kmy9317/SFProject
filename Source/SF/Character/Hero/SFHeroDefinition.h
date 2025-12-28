#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SFHeroDefinition.generated.h"

class USFPawnData;
class USkeletalMesh;

/**
 * 
 */
UCLASS()
class SF_API USFHeroDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(GetHeroDefinitionAssetType(), GetFName());
	}
	static FPrimaryAssetType GetHeroDefinitionAssetType() { return FPrimaryAssetType(TEXT("HeroDefinition")); }

	UFUNCTION(BlueprintPure, Category = "SF|Hero")
	FString GetHeroDisplayName() const { return HeroName; }

	UFUNCTION(BlueprintPure, Category = "SF|Hero")
	UTexture2D* LoadIcon() const;
	
	UFUNCTION(BlueprintPure, Category = "SF|Hero")
	TSoftObjectPtr<UTexture2D> GetIconPath() const { return HeroIcon; }

	UFUNCTION(BlueprintPure, Category = "SF|Hero")
	TSoftClassPtr<UAnimInstance> GetDisplayAnimBPPath() const { return DisplayAnimBP; }

	UFUNCTION(BlueprintPure, Category = "SF|Hero")
	TSubclassOf<UAnimInstance> LoadDisplayAnimationBP() const;

	UFUNCTION(BlueprintPure, Category = "SF|Hero")
	TSoftObjectPtr<USkeletalMesh> GetDisplayMeshPath() const { return DisplayMesh; }

	UFUNCTION(BlueprintPure, Category = "SF|Hero")
	USkeletalMesh* LoadDisplayMesh() const;

	UFUNCTION(BlueprintPure, Category = "SF|Hero")
	const USFPawnData* GetPawnData() const;

	UFUNCTION(BlueprintPure, Category = "SF|Hero")
	TSoftObjectPtr<USFPawnData> GetPawnDataPath() const { return PawnData; }

private:	
	UPROPERTY(EditDefaultsOnly, Category = "SF|Hero")
	FString HeroName;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Hero", meta = (AssetBundles = "Lobby"))
	TSoftObjectPtr<UTexture2D> HeroIcon;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Hero", meta = (AssetBundles = "Lobby"))
	TSoftObjectPtr<USkeletalMesh> DisplayMesh;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Hero", meta = (AssetBundles = "Lobby"))
	TSoftClassPtr<UAnimInstance> DisplayAnimBP;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Hero", meta = (AssetBundles = "InGame"))
	TSoftObjectPtr<USFPawnData> PawnData;
};
