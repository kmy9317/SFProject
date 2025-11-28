#include "SFAssetManager.h"

#include "SFLogChannels.h"
#include "Character/Hero/SFHeroDefinition.h"

void USFAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	// GameData 로드
	GetGameData();
	
}

void USFAssetManager::PreBeginPIE(bool bStartSimulate)
{
	Super::PreBeginPIE(bStartSimulate);

	GetGameData();
}

USFAssetManager& USFAssetManager::Get()
{
	USFAssetManager* Singleton = Cast<USFAssetManager>(GEngine->AssetManager.Get());
	if (Singleton)
	{
		return *Singleton;
	}

	UE_LOG(LogLoad, Fatal, TEXT("Asset Manager Needs to be of the type SFAssetMaanger"));
	return (*NewObject<USFAssetManager>());
}

const USFGameData& USFAssetManager::GetGameData()
{
	return GetOrLoadTypedGameData<USFGameData>(GameDataPath);
}

void USFAssetManager::LoadHeroDefinitions(const FStreamableDelegate& LoadFinishedCallback)
{
	LoadPrimaryAssetsWithType(USFHeroDefinition::GetHeroDefinitionAssetType(), TArray<FName>(), LoadFinishedCallback);
}

bool USFAssetManager::GetLoadedHeroDefinitions(TArray<USFHeroDefinition*>& LoadedHeroDefinations) const
{
	TArray<UObject*> LoadedObjects;
	bool bLoaded = GetPrimaryAssetObjectList(USFHeroDefinition::GetHeroDefinitionAssetType(), LoadedObjects);
	if (bLoaded)
	{
		for (UObject* LoadedObject : LoadedObjects)
		{
			LoadedHeroDefinations.Add(Cast<USFHeroDefinition>(LoadedObject));
			UE_LOG(LogLoad, Warning, TEXT("Loaded Hero Definition: %s"), *LoadedObject->GetName());
		}
	}

	return bLoaded;
}

TSharedPtr<FStreamableHandle> USFAssetManager::LoadPawnDataAsync(const TSoftObjectPtr<USFPawnData>& PawnDataPath, FStreamableDelegate& OnLoaded)
{
	if (PawnDataPath.IsNull())
	{
		return nullptr;
	}

	// RequestAsyncLoad 사용 - 특정 PawnData 하나만 빠르게 비동기 로드
	return GetStreamableManager().RequestAsyncLoad(PawnDataPath.ToSoftObjectPath(),OnLoaded);
}

UObject* USFAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
	if (AssetPath.IsValid())
	{
		if (UAssetManager::IsInitialized())
		{
			return UAssetManager::GetStreamableManager().LoadSynchronous(AssetPath, false);
		}

		// Use LoadObject if asset manager isn't ready yet.
		return AssetPath.TryLoad();
	}

	return nullptr;
}

void USFAssetManager::AddLoadedAsset(const UObject* Asset)
{
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		LoadedAssets.Add(Asset);
	}
}

UPrimaryDataAsset* USFAssetManager::LoadGameDataOfClass(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType)
{
	UPrimaryDataAsset* Asset = nullptr;

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading GameData Object"), STAT_GameData, STATGROUP_LoadTime);
	if (!DataClassPath.IsNull())
	{
#if WITH_EDITOR
		FScopedSlowTask SlowTask(0, FText::Format(NSLOCTEXT("SFEditor", "BeginLoadingGameDataTask", "Loading GameData {0}"), FText::FromName(DataClass->GetFName())));
		const bool bShowCancelButton = false;
		const bool bAllowInPIE = true;
		SlowTask.MakeDialog(bShowCancelButton, bAllowInPIE);
#endif
		UE_LOG(LogSF, Log, TEXT("Loading GameData: %s ..."), *DataClassPath.ToString());
		SCOPE_LOG_TIME_IN_SECONDS(TEXT("    ... GameData loaded!"), nullptr);

		// This can be called recursively in the editor because it is called on demand from PostLoad so force a sync load for primary asset and async load the rest in that case
		if (GIsEditor)
		{
			Asset = DataClassPath.LoadSynchronous();
			LoadPrimaryAssetsWithType(PrimaryAssetType);
		}
		else
		{
			TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssetsWithType(PrimaryAssetType);
			if (Handle.IsValid())
			{
				Handle->WaitUntilComplete(0.0f, false);

				// This should always work
				Asset = Cast<UPrimaryDataAsset>(Handle->GetLoadedAsset());
			}
		}
	}

	if (Asset)
	{
		GameDataMap.Add(DataClass, Asset);
	}
	else
	{
		// It is not acceptable to fail to load any GameData asset. It will result in soft failures that are hard to diagnose.
		UE_LOG(LogSF, Fatal, TEXT("Failed to load GameData asset at %s. Type %s. This is not recoverable and likely means you do not have the correct data to run %s."), *DataClassPath.ToString(), *PrimaryAssetType.ToString(), FApp::GetProjectName());
	}

	return Asset;
}
