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
    UpdateStageInfoFromLevel(MapName);
    UpdateAssetBundlesForLevel(MapName);
}

void USFStageSubsystem::OnSeamlessTravelStart(UWorld* CurrentWorld, const FString& LevelName)
{
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
    const FSFStageConfig* Config = GetStageConfigForLevel(LevelName);
    ESFLevelType LevelType = Config ? Config->LevelType : ESFLevelType::Menu;
    
    // 현재 로드된 번들과 비교하여 필요한 것만 로드/언로드(이미 로드된 상태면 스킵)
    switch (LevelType)
    {
    case ESFLevelType::InGame:
        if (!AssetManager.AreInGameAssetsLoaded())
        {
            AssetManager.LoadInGameAssets();
        }
        // Lobby 번들은 유지 (InGame에서도 Hero 정보 필요할 수 있음)
        break;
        
    case ESFLevelType::Lobby:
        // InGame 번들 언로드, Lobby 유지
        AssetManager.UnloadInGameAssets();
        if (!AssetManager.AreLobbyAssetsLoaded())
        {
            AssetManager.LoadLobbyAssets();
        }
        break;
        
    case ESFLevelType::Menu:
        // 모든 번들 언로드 (메모리 최소화)
        AssetManager.UnloadInGameAssets();
        // Lobby는 필요시 언로드 
        // AssetManager.UnloadLobbyAssets();
        break;
    }
}
