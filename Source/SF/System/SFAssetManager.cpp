#include "SFAssetManager.h"

#include "SFLogChannels.h"
#include "Character/Hero/SFHeroDefinition.h"
#include "Data/Common/SFCommonLootTable.h"
#include "Data/Common/SFCommonRarityConfig.h"
#include "Data/Common/SFCommonUpgradeDefinition.h"

void USFAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	// GameData 로드
	GetGameData();

}

void USFAssetManager::FinishInitialLoading()
{
	Super::FinishInitialLoading();
}

void USFAssetManager::PostInitialAssetScan()
{
	Super::PostInitialAssetScan();

	LoadAllPrimaryAssets();
}

#if WITH_EDITOR
void USFAssetManager::PreBeginPIE(bool bStartSimulate)
{
	Super::PreBeginPIE(bStartSimulate);

	GetGameData();
	if (!AreLobbyAssetsLoaded())
	{
		LoadAllPrimaryAssets();
	}
}
#endif

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

TArray<FPrimaryAssetType> USFAssetManager::GetManagedPrimaryAssetTypes() const
{
	TArray<FPrimaryAssetType> Types;

    // HeroDefinition PrimaryDataAsset
	Types.Add(USFHeroDefinition::GetHeroDefinitionAssetType());

    // CommonUpgrade PrimaryDataAssets
    Types.Add(USFCommonUpgradeDefinition::GetCommonUpgradeDefinitionAssetType());
    Types.Add(USFCommonRarityConfig::GetCommonRarityConfigAssetType());
    Types.Add(USFCommonLootTable::GetCommonLootTableAssetType());
	return Types;
}

//////////////////////////////////////////////////////////////////////////
//                          초기 로딩
//////////////////////////////////////////////////////////////////////////

void USFAssetManager::LoadAllPrimaryAssets()
{
#if WITH_EDITOR
    FScopedSlowTask SlowTask(0, NSLOCTEXT("SFEditor", "LoadingAssets", "Loading Primary Assets..."));
    SlowTask.MakeDialog(false, true);
#endif

    UE_LOG(LogSF, Log, TEXT("Loading All PrimaryAssets..."));
    SCOPE_LOG_TIME_IN_SECONDS(TEXT("    ... PrimaryAssets loaded!"), nullptr);

    TArray<FPrimaryAssetType> ManagedTypes = GetManagedPrimaryAssetTypes();

    // 1단계: 모든 PrimaryAsset 타입 동기 로드
    for (const FPrimaryAssetType& AssetType : ManagedTypes)
    {
        TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssetsWithType(AssetType);
        if (Handle.IsValid())
        {
            Handle->WaitUntilComplete(0.0f, false);
            PrimaryAssetHandles.Add(AssetType, Handle);

            TArray<FPrimaryAssetId> AssetIds;
            GetPrimaryAssetIdList(AssetType, AssetIds);
            UE_LOG(LogSF, Log, TEXT("  - %s: %d assets loaded"), *AssetType.ToString(), AssetIds.Num());
        }
    }

    // 2단계: Lobby 번들 동기 로드(현재는 메인 메뉴 진입시에 Lobby에 필요한 에셋 번들 로드)
    TArray<FPrimaryAssetId> AllAssetIds;
    for (const FPrimaryAssetType& AssetType : ManagedTypes)
    {
        TArray<FPrimaryAssetId> TypeAssetIds;
        GetPrimaryAssetIdList(AssetType, TypeAssetIds);
        AllAssetIds.Append(TypeAssetIds);
    }

    if (AllAssetIds.Num() > 0)
    {
        TArray<FName> BundlesToLoad;
        BundlesToLoad.Add(TEXT("Lobby"));

        TSharedPtr<FStreamableHandle> LobbyHandle = LoadPrimaryAssets(AllAssetIds, BundlesToLoad);
        if (LobbyHandle.IsValid())
        {
            LobbyHandle->WaitUntilComplete(0.0f, false);
        }
        BundleHandles.Add(TEXT("Lobby"), LobbyHandle);
        BundleAssetTypes.Add(TEXT("Lobby"), ManagedTypes);

        OnBundleLoaded(TEXT("Lobby"));
    }
}

//////////////////////////////////////////////////////////////////////////
//                    PrimaryAsset 타입 관리
//////////////////////////////////////////////////////////////////////////

void USFAssetManager::LoadPrimaryAssetType(FPrimaryAssetType AssetType, const FStreamableDelegate& OnComplete)
{
    if (IsPrimaryAssetTypeLoaded(AssetType))
    {
        OnComplete.ExecuteIfBound();
        return;
    }

    if (OnComplete.IsBound())
    {
        PendingPrimaryAssetCallbacks.Add(AssetType, OnComplete);
    }

    TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssetsWithType(AssetType);
    if (Handle.IsValid())
    {
        Handle->BindCompleteDelegate(FStreamableDelegate::CreateUObject(this, &USFAssetManager::OnPrimaryAssetTypeLoaded, AssetType));
        PrimaryAssetHandles.Add(AssetType, Handle);
    }
}

void USFAssetManager::OnPrimaryAssetTypeLoaded(FPrimaryAssetType AssetType)
{
    if (FStreamableDelegate* Callback = PendingPrimaryAssetCallbacks.Find(AssetType))
    {
        Callback->ExecuteIfBound();
        PendingPrimaryAssetCallbacks.Remove(AssetType);
    }
}

void USFAssetManager::UnloadPrimaryAssetType(FPrimaryAssetType AssetType)
{
    if (TSharedPtr<FStreamableHandle>* Handle = PrimaryAssetHandles.Find(AssetType))
    {
        if (Handle->IsValid())
        {
            UE_LOG(LogSF, Log, TEXT("Unloading PrimaryAssetType [%s]..."), *AssetType.ToString());
            (*Handle)->ReleaseHandle();
            Handle->Reset();
        }
        PrimaryAssetHandles.Remove(AssetType);
    }
}

bool USFAssetManager::IsPrimaryAssetTypeLoaded(FPrimaryAssetType AssetType) const
{
    const TSharedPtr<FStreamableHandle>* Handle = PrimaryAssetHandles.Find(AssetType);
    return Handle && Handle->IsValid() && (*Handle)->HasLoadCompleted();
}

//////////////////////////////////////////////////////////////////////////
//                          Bundle 관리
//////////////////////////////////////////////////////////////////////////

void USFAssetManager::LoadBundle(FName BundleName, const TArray<FPrimaryAssetType>& AssetTypes, const FStreamableDelegate& OnComplete)
{
    if (IsBundleLoaded(BundleName))
    {
        UE_LOG(LogSF, Log, TEXT("Bundle [%s] already loaded."), *BundleName.ToString());
        OnComplete.ExecuteIfBound();
        return;
    }

    UE_LOG(LogSF, Log, TEXT("Loading Bundle [%s]..."), *BundleName.ToString());

    // 콜백 및 타입 정보 저장
    if (OnComplete.IsBound())
    {
        PendingBundleCallbacks.Add(BundleName, OnComplete);
    }
    BundleAssetTypes.Add(BundleName, AssetTypes);

    // 모든 PrimaryAsset 타입이 로드되었는지 확인
    bool bAllTypesLoaded = true;
    for (const FPrimaryAssetType& AssetType : AssetTypes)
    {
        if (!IsPrimaryAssetTypeLoaded(AssetType))
        {
            bAllTypesLoaded = false;
            break;
        }
    }

    // PrimaryAsset ID 수집
    TArray<FPrimaryAssetId> AllAssetIds;
    for (const FPrimaryAssetType& AssetType : AssetTypes)
    {
        TArray<FPrimaryAssetId> TypeAssetIds;
        GetPrimaryAssetIdList(AssetType, TypeAssetIds);
        AllAssetIds.Append(TypeAssetIds);
    }

    if (AllAssetIds.Num() == 0)
    {
        UE_LOG(LogSF, Warning, TEXT("No assets found for bundle [%s]!"), *BundleName.ToString());
        OnComplete.ExecuteIfBound();
        return;
    }

    TArray<FName> BundlesToLoad;
    BundlesToLoad.Add(BundleName);

    if (bAllTypesLoaded)
    {
        // 바로 번들 로드
        TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssets(AllAssetIds, BundlesToLoad, FStreamableDelegate::CreateUObject(this, &USFAssetManager::OnBundleLoaded, BundleName));
        BundleHandles.Add(BundleName, Handle);
    }
    else
    {
        // PrimaryAsset 타입들 먼저 로드
        PendingAssetTypesForBundle.Add(BundleName, AssetTypes.Num());
        for (const FPrimaryAssetType& AssetType : AssetTypes)
        {
            if (!IsPrimaryAssetTypeLoaded(AssetType))
            {
                LoadPrimaryAssetType(AssetType, FStreamableDelegate::CreateLambda([this, BundleName, AllAssetIds, BundlesToLoad]()
                {
                    int32& PendingCount = PendingAssetTypesForBundle.FindChecked(BundleName);
                    PendingCount--;

                    if (PendingCount <= 0)
                    {
                        PendingAssetTypesForBundle.Remove(BundleName);
                        TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssets(AllAssetIds, BundlesToLoad, FStreamableDelegate::CreateUObject(this, &USFAssetManager::OnBundleLoaded, BundleName));
                        BundleHandles.Add(BundleName, Handle);
                    }
                }));
            }
            else
            {
                int32& PendingCount = PendingAssetTypesForBundle.FindChecked(BundleName);
                PendingCount--;
            }
        }
    }
}

void USFAssetManager::OnBundleLoaded(FName BundleName)
{
    UE_LOG(LogSF, Log, TEXT("Bundle [%s] load complete!"), *BundleName.ToString());

    // Lobby 번들 디버그용 로그
    if (BundleName == TEXT("Lobby"))
    {
        TArray<USFHeroDefinition*> LoadedDefs;
        GetLoadedHeroDefinitions(LoadedDefs);

        for (const USFHeroDefinition* Def : LoadedDefs)
        {
            UE_LOG(LogSF, Log, TEXT("  - %s: Icon=%s, Mesh=%s, AnimBP=%s"),
                *Def->GetHeroDisplayName(),
                Def->GetIconPath().Get() ? TEXT("OK") : TEXT("NULL"),
                Def->GetDisplayMeshPath().Get() ? TEXT("OK") : TEXT("NULL"),
                Def->GetDisplayAnimBPPath().Get() ? TEXT("OK") : TEXT("NULL"));
        }
    }

    // InGame 번들 디버그용 로그
    if (BundleName == TEXT("InGame"))
    {
        // CommonUpgradeDefinition 확인
        TArray<UObject*> UpgradeObjects;
        GetPrimaryAssetObjectList(USFCommonUpgradeDefinition::GetCommonUpgradeDefinitionAssetType(), UpgradeObjects);
        
        for (UObject* Obj : UpgradeObjects)
        {
            if (const USFCommonUpgradeDefinition* Def = Cast<USFCommonUpgradeDefinition>(Obj))
            {
                UE_LOG(LogSF, Log, TEXT("  - Upgrade [%s]: Icon=%s"),
                    *Def->DisplayName.ToString(),
                    Def->Icon.Get() ? TEXT("OK") : TEXT("NULL"));
            }
        }
        
        // CommonRarityConfig 확인
        TArray<UObject*> RarityObjects;
        GetPrimaryAssetObjectList(USFCommonRarityConfig::GetCommonRarityConfigAssetType(), RarityObjects);
        
        for (UObject* Obj : RarityObjects)
        {
            if (const USFCommonRarityConfig* Config = Cast<USFCommonRarityConfig>(Obj))
            {
                UE_LOG(LogSF, Log, TEXT("  - Rarity [%s]: Frame=%s, Curve=%s"),
                    *Config->RarityTag.ToString(),
                    Config->FrameTexture.Get() ? TEXT("OK") : TEXT("NULL"),
                    Config->LuckWeightCurve.Get() ? TEXT("OK") : TEXT("NULL"));
            }
        }
    }

    if (FStreamableDelegate* Callback = PendingBundleCallbacks.Find(BundleName))
    {
        Callback->ExecuteIfBound();
        PendingBundleCallbacks.Remove(BundleName);
    }
}

void USFAssetManager::UnloadBundle(FName BundleName)
{
    if (TSharedPtr<FStreamableHandle>* Handle = BundleHandles.Find(BundleName))
    {
        if (Handle->IsValid())
        {
            UE_LOG(LogSF, Log, TEXT("Unloading Bundle [%s]..."), *BundleName.ToString());
            (*Handle)->ReleaseHandle();
            Handle->Reset();
        }
        BundleHandles.Remove(BundleName);
    }
    BundleAssetTypes.Remove(BundleName);
}

bool USFAssetManager::IsBundleLoaded(FName BundleName) const
{
    const TSharedPtr<FStreamableHandle>* Handle = BundleHandles.Find(BundleName);
    return Handle && Handle->IsValid() && (*Handle)->HasLoadCompleted();
}

//////////////////////////////////////////////////////////////////////////
//                        Load Asset Helper
//////////////////////////////////////////////////////////////////////////

void USFAssetManager::LoadLobbyAssets(const FStreamableDelegate& OnComplete)
{
    LoadBundle(TEXT("Lobby"), GetManagedPrimaryAssetTypes(), OnComplete);
}

void USFAssetManager::LoadInGameAssets(const FStreamableDelegate& OnComplete)
{
    LoadBundle(TEXT("InGame"), GetManagedPrimaryAssetTypes(), OnComplete);
}

//////////////////////////////////////////////////////////////////////////
//                  HeroDefinition (기존 API 호환)
//////////////////////////////////////////////////////////////////////////

void USFAssetManager::LoadHeroDefinitions(const FStreamableDelegate& LoadFinishedCallback)
{
    if (AreLobbyAssetsLoaded())
    {
        LoadFinishedCallback.ExecuteIfBound();
    }
    else
    {
        LoadLobbyAssets(LoadFinishedCallback);
    }
}

bool USFAssetManager::GetLoadedHeroDefinitions(TArray<USFHeroDefinition*>& OutHeroDefinitions) const
{
    TArray<UObject*> LoadedObjects;
    bool bLoaded = GetPrimaryAssetObjectList(USFHeroDefinition::GetHeroDefinitionAssetType(), LoadedObjects);

    if (bLoaded)
    {
        for (UObject* LoadedObject : LoadedObjects)
        {
            if (USFHeroDefinition* HeroDef = Cast<USFHeroDefinition>(LoadedObject))
            {
                OutHeroDefinitions.Add(HeroDef);
            }
        }
    }
    return OutHeroDefinitions.Num() > 0;
}

TSharedPtr<FStreamableHandle> USFAssetManager::LoadPawnDataAsync(const TSoftObjectPtr<USFPawnData>& PawnDataPath, FStreamableDelegate& OnLoaded)
{
    if (PawnDataPath.IsNull())
    {
        return nullptr;
    }
    return GetStreamableManager().RequestAsyncLoad(PawnDataPath.ToSoftObjectPath(), OnLoaded);
}

//////////////////////////////////////////////////////////////////////////
// Utility
//////////////////////////////////////////////////////////////////////////

UObject* USFAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
    if (AssetPath.IsValid())
    {
        if (UAssetManager::IsInitialized())
        {
            return UAssetManager::GetStreamableManager().LoadSynchronous(AssetPath, false);
        }
        return AssetPath.TryLoad();
    }
    return nullptr;
}

void USFAssetManager::AddLoadedAsset(const UObject* Asset)
{
    if (ensureAlways(Asset))
    {
        FScopeLock Lock(&LoadedAssetsCritical);
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
        SlowTask.MakeDialog(false, true);
#endif
        UE_LOG(LogSF, Log, TEXT("Loading GameData: %s ..."), *DataClassPath.ToString());
        SCOPE_LOG_TIME_IN_SECONDS(TEXT("    ... GameData loaded!"), nullptr);

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
        UE_LOG(LogSF, Fatal, TEXT("Failed to load GameData asset at %s. Type %s. This is not recoverable and likely means you do not have the correct data to run %s."),
            *DataClassPath.ToString(), *PrimaryAssetType.ToString(), FApp::GetProjectName());
    }

    return Asset;
}
