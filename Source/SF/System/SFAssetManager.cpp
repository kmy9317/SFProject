#include "SFAssetManager.h"

#include "Character/Hero/SFHeroDefinition.h"

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