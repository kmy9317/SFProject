// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "System/Data/SFGameData.h"
#include "SFAssetManager.generated.h"

class USFGameData;
class USFPawnData;
class USFHeroDefinition;

/**
 * 
 */
UCLASS(Config=Game)
class SF_API USFAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:

	//~UAssetManager interface
	virtual void StartInitialLoading() override;
#if WITH_EDITOR
	virtual void PreBeginPIE(bool bStartSimulate) override;
#endif
	//~End of UAssetManager interface
	
	static USFAssetManager& Get();

	template<typename AssetType>
	static TSubclassOf<AssetType> GetSubclassByPath(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	const USFGameData& GetGameData();
	
	void LoadHeroDefinitions(const FStreamableDelegate& LoadFinishedCallback);
	bool GetLoadedHeroDefinitions(TArray<USFHeroDefinition*>& LoadedHeroDefinations) const;

	TSharedPtr<FStreamableHandle> LoadPawnDataAsync(const TSoftObjectPtr<USFPawnData>& PawnDataPath, FStreamableDelegate& LoadCompleteDelegate);

protected:
	template <typename GameDataClass>
	const GameDataClass& GetOrLoadTypedGameData(const TSoftObjectPtr<GameDataClass>& DataPath)
	{
		if (TObjectPtr<UPrimaryDataAsset> const * pResult = GameDataMap.Find(GameDataClass::StaticClass()))
		{
			return *CastChecked<GameDataClass>(*pResult);
		}

		// Does a blocking load if needed
		return *CastChecked<const GameDataClass>(LoadGameDataOfClass(GameDataClass::StaticClass(), DataPath, GameDataClass::StaticClass()->GetFName()));
	}

	static UObject* SynchronousLoadAsset(const FSoftObjectPath& AssetPath);
	
	// Thread safe way of adding a loaded asset to keep in memory.
	void AddLoadedAsset(const UObject* Asset);
	
	UPrimaryDataAsset* LoadGameDataOfClass(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType);

protected:
	
	UPROPERTY(Config)
	TSoftObjectPtr<USFGameData> GameDataPath;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UPrimaryDataAsset>> GameDataMap;

	// Assets loaded and tracked by the asset manager.
	UPROPERTY()
	TSet<TObjectPtr<const UObject>> LoadedAssets;

	// Used for a scope lock when modifying the list of load assets.
	FCriticalSection LoadedAssetsCritical;
};

template <typename AssetType>
TSubclassOf<AssetType> USFAssetManager::GetSubclassByPath(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	TSubclassOf<AssetType> LoadedSubclass;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedSubclass = AssetPointer.Get();
		if (!LoadedSubclass)
		{
			LoadedSubclass = Cast<UClass>(SynchronousLoadAsset(AssetPath));
			ensureAlwaysMsgf(LoadedSubclass, TEXT("Failed to load asset class [%s]"), *AssetPointer.ToString());
		}

		if (LoadedSubclass && bKeepInMemory)
		{
			// Added to loaded asset list.
			Get().AddLoadedAsset(Cast<UObject>(LoadedSubclass));
		}
	}

	return LoadedSubclass;
}
