// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "System/Data/SFGameData.h"
#include "Item/SFItemData.h"
#include "SFAssetManager.generated.h"

class USFItemData;
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
	virtual void FinishInitialLoading() override;
	virtual void PostInitialAssetScan() override;
#if WITH_EDITOR
	virtual void PreBeginPIE(bool bStartSimulate) override;
#endif
	//~End of UAssetManager interface
	
	static USFAssetManager& Get();

	template<typename AssetType>
	static AssetType* GetAssetByPath(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	template<typename AssetType>
	static TSubclassOf<AssetType> GetSubclassByPath(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	const USFGameData& GetGameData();
	const USFItemData& GetItemData();

	// HeroDefinition 로드 요청 (이미 로드되어 있으면 콜백 즉시 실행)
	void LoadHeroDefinitions(const FStreamableDelegate& LoadFinishedCallback);
	bool GetLoadedHeroDefinitions(TArray<USFHeroDefinition*>& OutHeroDefinations) const;
	TSharedPtr<FStreamableHandle> LoadPawnDataAsync(const TSoftObjectPtr<USFPawnData>& PawnDataPath, FStreamableDelegate& LoadCompleteDelegate);

	// ===== PrimaryAsset 관리 =====
	void LoadPrimaryAssetType(FPrimaryAssetType AssetType, const FStreamableDelegate& OnComplete = FStreamableDelegate());
	void UnloadPrimaryAssetType(FPrimaryAssetType AssetType);
	bool IsPrimaryAssetTypeLoaded(FPrimaryAssetType AssetType) const;
	
	// ===== Bundle 관리 =====
	void LoadBundle(FName BundleName, const TArray<FPrimaryAssetType>& AssetTypes, const FStreamableDelegate& OnComplete = FStreamableDelegate());
	void UnloadBundle(FName BundleName);
	bool IsBundleLoaded(FName BundleName) const;

	// Lobby 에셋 관련 래퍼 함수들
	void LoadLobbyAssets(const FStreamableDelegate& OnComplete = FStreamableDelegate());
	void UnloadLobbyAssets() { UnloadBundle(TEXT("Lobby")); }
	bool AreLobbyAssetsLoaded() const { return IsBundleLoaded(TEXT("Lobby")); }

	void LoadInGameAssets(const FStreamableDelegate& OnComplete = FStreamableDelegate());
	void UnloadInGameAssets() { UnloadBundle(TEXT("InGame")); }
	bool AreInGameAssetsLoaded() const { return IsBundleLoaded(TEXT("InGame")); }
	

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
	
private:
	
	void LoadAllPrimaryAssets();
	void OnPrimaryAssetTypeLoaded(FPrimaryAssetType AssetType);
	void OnBundleLoaded(FName BundleName);
	TArray<FPrimaryAssetType> GetManagedPrimaryAssetTypes() const;

protected:
	
	UPROPERTY(Config)
	TSoftObjectPtr<USFGameData> GameDataPath;

	UPROPERTY(Config)
	TSoftObjectPtr<USFItemData> ItemDataPath;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UPrimaryDataAsset>> GameDataMap;

	// Assets loaded and tracked by the asset manager.
	UPROPERTY()
	TSet<TObjectPtr<const UObject>> LoadedAssets;

	// Used for a scope lock when modifying the list of load assets.
	FCriticalSection LoadedAssetsCritical;

private:
	// ===== PrimaryAsset Handles (타입별 관리) =====
	TMap<FPrimaryAssetType, TSharedPtr<FStreamableHandle>> PrimaryAssetHandles;
	TMap<FPrimaryAssetType, FStreamableDelegate> PendingPrimaryAssetCallbacks;

	// ===== Bundle Handles =====
	TMap<FName, TSharedPtr<FStreamableHandle>> BundleHandles;
	TMap<FName, FStreamableDelegate> PendingBundleCallbacks;
    
	// 번들 로드 시 대기 중인 PrimaryAsset 타입 수
	TMap<FName, int32> PendingAssetTypesForBundle;
	TMap<FName, TArray<FPrimaryAssetType>> BundleAssetTypes;
};

template<typename AssetType>
AssetType* USFAssetManager::GetAssetByPath(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	AssetType* LoadedAsset = nullptr;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedAsset = AssetPointer.Get();
		if (!LoadedAsset)
		{
			LoadedAsset = Cast<AssetType>(SynchronousLoadAsset(AssetPath));
			ensureAlwaysMsgf(LoadedAsset, TEXT("Failed to load asset [%s]"), *AssetPointer.ToString());
		}

		if (LoadedAsset && bKeepInMemory)
		{
			// Added to loaded asset list.
			Get().AddLoadedAsset(Cast<UObject>(LoadedAsset));
		}
	}

	return LoadedAsset;
}

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
