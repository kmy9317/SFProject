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

	FString GetHeroDisplayName() const { return HeroName; }
	UTexture2D* LoadIcon() const;
	TSubclassOf<UAnimInstance> LoadDisplayAnimationBP() const;
	USkeletalMesh* LoadDisplayMesh() const;
	const USFPawnData* GetPawnData() const;
	TSoftObjectPtr<USFPawnData> GetPawnDataPath() const { return PawnData; }

private:	
	UPROPERTY(EditDefaultsOnly, Category = "SF|Hero")
	FString HeroName;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Hero")
	TSoftObjectPtr<UTexture2D> HeroIcon;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Hero")
	TSoftObjectPtr<USkeletalMesh> DisplayMesh;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Hero")
	TSoftClassPtr<UAnimInstance> DisplayAnimBP;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Hero")
	TSoftObjectPtr<USFPawnData> PawnData;
};
