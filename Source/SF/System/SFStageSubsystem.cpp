#include "SFStageSubsystem.h"

#include "SFAssetManager.h"
#include "SFLogChannels.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

void USFStageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    CurrentStageInfo = FSFStageInfo();

    // DataTable 비동기 로드
    if (!StageConfigTable.IsNull())
    {
        FStreamableDelegate OnLoaded = FStreamableDelegate::CreateUObject(
            this, &USFStageSubsystem::OnConfigTableLoaded);
        
        ConfigTableLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
            StageConfigTable.ToSoftObjectPath(),
            OnLoaded,
            FStreamableManager::AsyncLoadHighPriority
        );
    }
    
    // Hard Travel 감지
    FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &USFStageSubsystem::OnPreLoadMap);
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USFStageSubsystem::OnPostLoadMapWithWorld);

    // Seamless Travel 감지
    FWorldDelegates::OnSeamlessTravelStart.AddUObject(this, &USFStageSubsystem::OnSeamlessTravelStart);
}

void USFStageSubsystem::Deinitialize()
{
    FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
    FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
    FWorldDelegates::OnSeamlessTravelStart.RemoveAll(this);

    ConfigTableLoadHandle.Reset();
    
    Super::Deinitialize();
}

bool USFStageSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    // 서버와 클라이언트 모두에서 생성
    return true;
}

void USFStageSubsystem::OnPreLoadMap(const FString& MapName)
{
    CurrentTravelType = ESFTravelType::Hard;
    UpdateStageInfoFromLevel(MapName);
    UpdateAssetBundlesForLevel(MapName);
}

void USFStageSubsystem::OnSeamlessTravelStart(UWorld* CurrentWorld, const FString& LevelName)
{
    CurrentTravelType = ESFTravelType::Seamless;
    UpdateStageInfoFromLevel(LevelName);
    UpdateAssetBundlesForLevel(LevelName);
}

void USFStageSubsystem::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
   
}

void USFStageSubsystem::SetCurrentStageInfo(const FSFStageInfo& NewStageInfo)
{
    if (CurrentStageInfo != NewStageInfo)
    {
        CurrentStageInfo = NewStageInfo;
    }
}

void USFStageSubsystem::ResetStageInfo()
{
    CurrentStageInfo = FSFStageInfo();
}

void USFStageSubsystem::SetPlayerCount(int32 Count)
{
    PlayerCount = FMath::Max(1, Count);
}

FSFStageInfo USFStageSubsystem::GetStageInfoForLevel(const FString& LevelName) const
{
    if (const FSFStageConfig* Config = GetStageConfigForLevel(LevelName))
    {
        return Config->StageInfo;
    }
    return FSFStageInfo();

}

const FSFStageConfig* USFStageSubsystem::GetStageConfigForLevel(const FString& LevelName) const
{
    UDataTable* ConfigTable = CachedConfigTable;
    
    // 아직 로드 안됐으면 동기 로드 (폴백)
    if (!ConfigTable)
    {
        ConfigTable = StageConfigTable.LoadSynchronous();
    }
    
    if (!ConfigTable)
    {
        UE_LOG(LogSF, Warning, TEXT("[StageSubsystem] StageConfigTable is not set!"));
        return nullptr;
    }

    FString ShortName = FPackageName::GetShortName(LevelName);
    return ConfigTable->FindRow<FSFStageConfig>(FName(*ShortName), TEXT("GetStageConfigForLevel"));
}

void USFStageSubsystem::UpdateStageInfoFromLevel(const FString& LevelName)
{
    if (const FSFStageConfig* Config = GetStageConfigForLevel(LevelName))
    {
        SetCurrentStageInfo(Config->StageInfo);
    }
}

void USFStageSubsystem::OnConfigTableLoaded()
{
    CachedConfigTable = StageConfigTable.Get();

    if (CachedConfigTable)
    {
        UE_LOG(LogSF, Log, TEXT("[StageSubsystem] Config table loaded successfully"));
    }
    else
    {
        UE_LOG(LogSF, Error, TEXT("[StageSubsystem] Failed to load config table!"));
    }
    
    ConfigTableLoadHandle.Reset();
}

void USFStageSubsystem::UpdateAssetBundlesForLevel(const FString& LevelName)
{
    FString ShortName = FPackageName::GetShortName(LevelName);
    
    // 중복 호출 방지
    if (LastProcessedLevelForBundles == ShortName)
    {
        return;
    }
    LastProcessedLevelForBundles = ShortName;
    
    USFAssetManager& AssetManager = USFAssetManager::Get();

    // 이전 번들 로드 요청 취소
    AssetManager.CancelPendingBundleLoads();
    
    const FSFStageConfig* Config = GetStageConfigForLevel(LevelName);
    ESFLevelType LevelType = Config ? Config->LevelType : ESFLevelType::Menu;
    
    // 현재 로드된 번들과 비교하여 필요한 것만 로드/언로드(이미 로드된 상태면 스킵)
    switch (LevelType)
    {
    case ESFLevelType::InGame:
        if (!AssetManager.AreInGameAssetsLoaded())
        {
            BroadcastBundleLoadStarted();
            AssetManager.LoadInGameAssets(FStreamableDelegate::CreateUObject(this, &ThisClass::OnBundleLoadComplete));
        }
        else
        {
            BroadcastBundlesReady();
        }
        AssetManager.UnloadLobbyAssets();
        break;
    case ESFLevelType::Lobby:
        if (!AssetManager.AreLobbyAssetsLoaded())
        {
            BroadcastBundleLoadStarted();
            AssetManager.LoadLobbyAssets(FStreamableDelegate::CreateUObject(this, &ThisClass::OnBundleLoadComplete));
        }
        else
        {
            BroadcastBundlesReady();
        }
        AssetManager.UnloadInGameAssets();
        break;
    case ESFLevelType::Menu:
        AssetManager.UnloadInGameAssets();
        AssetManager.UnloadLobbyAssets();
        BroadcastBundlesReady();
        break;
    }
}

void USFStageSubsystem::BroadcastBundleLoadStarted()
{
    if (CurrentTravelType == ESFTravelType::Hard)
    {
        OnHardTravelBundleLoadStarted.Broadcast();
    }
    else if (CurrentTravelType == ESFTravelType::Seamless)
    {
        OnSeamlessTravelBundleLoadStarted.Broadcast();
    }
}

void USFStageSubsystem::BroadcastBundlesReady()
{
    if (CurrentTravelType == ESFTravelType::Hard)
    {
        OnHardTravelBundlesReady.Broadcast();
    }
    else if (CurrentTravelType == ESFTravelType::Seamless)
    {
        OnSeamlessTravelBundlesReady.Broadcast();
    }
    CurrentTravelType = ESFTravelType::None;
}

void USFStageSubsystem::OnBundleLoadComplete()
{
    BroadcastBundlesReady();
}
