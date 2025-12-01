#include "SFLoadingScreenSubsystem.h"
#include "MoviePlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "SFLogChannels.h"
#include "Blueprint/UserWidget.h"
#include "Engine/AssetManager.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StreamableManager.h"
#include "Engine/UserInterfaceSettings.h"
#include "UI/Slate/SFSimpleLoadingScreen.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Widgets/Layout/SSafeZone.h"

void USFLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

    if (!LoadingConfigTable.IsNull())
    {
        FStreamableDelegate OnLoaded = FStreamableDelegate::CreateUObject(
            this, &USFLoadingScreenSubsystem::OnConfigTableLoaded);
        
        ConfigTableLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
            LoadingConfigTable.ToSoftObjectPath(),
            OnLoaded,
            FStreamableManager::AsyncLoadHighPriority
        );
    }
    
    // 하드 트래블(OpenLevel) 감지
    FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &USFLoadingScreenSubsystem::OnPreLoadMap);

    // 맵 로드 완료 감지 
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USFLoadingScreenSubsystem::OnPostLoadMapWithWorld);

    // 심리스 트레블 감지(로딩 스크린 설정용)
    FWorldDelegates::OnSeamlessTravelStart.AddUObject(this, &USFLoadingScreenSubsystem::OnSeamlessTravelStart);
    
    bCurrentLoadingScreenStarted = false;
}

void USFLoadingScreenSubsystem::Deinitialize()
{
    RemoveWidgetFromViewport();
    
    FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
    FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
    FWorldDelegates::OnSeamlessTravelStart.RemoveAll(this);

    ConfigTableLoadHandle.Reset();
    CurrentWidgetLoadHandle.Reset();
    
	Super::Deinitialize();
}

bool USFLoadingScreenSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UGameInstance* GameInstance = CastChecked<UGameInstance>(Outer);
    const bool bIsServerWorld = GameInstance->IsDedicatedServerInstance();
    return !bIsServerWorld;
}

void USFLoadingScreenSubsystem::PreloadLoadingScreenForLevel(const FString& NextLevelName)
{
    PendingLevelName = NextLevelName;
    PreloadedWidgetClass = nullptr; // 리셋
    
    const FSFMapLoadingConfig* Config = FindLoadingConfigForMap(NextLevelName);

    if (Config && !Config->SeamlessLoadingWidget.IsNull())
    {
        // 기존 핸들 해제 (이전 로딩 화면 메모리 해제)
        if (CurrentWidgetLoadHandle.IsValid())
        {
            CurrentWidgetLoadHandle->ReleaseHandle();
            CurrentWidgetLoadHandle.Reset();
        }

        // 비동기 로딩 시작
        TSoftClassPtr<UUserWidget> WidgetPath = Config->SeamlessLoadingWidget;
        
        CurrentWidgetLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
            WidgetPath.ToSoftObjectPath(),
            FStreamableDelegate::CreateUObject(this, &USFLoadingScreenSubsystem::OnLoadingScreenAssetLoaded),
            FStreamableManager::AsyncLoadHighPriority
        );
    }
    else if (Config && !Config->HardLoadingWidget.IsNull())
    {
        if (CurrentWidgetLoadHandle.IsValid())
        {
            CurrentWidgetLoadHandle->ReleaseHandle();
            CurrentWidgetLoadHandle.Reset();
        }

        TSoftClassPtr<UUserWidget> WidgetPath = Config->HardLoadingWidget;
        
        CurrentWidgetLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
            WidgetPath.ToSoftObjectPath(),
            FStreamableDelegate::CreateUObject(this, &USFLoadingScreenSubsystem::OnLoadingScreenAssetLoaded),
            FStreamableManager::AsyncLoadHighPriority
        );
    }
    else
    {
        UE_LOG(LogSF, Warning, TEXT("[Preload] No config found for level: %s"), *NextLevelName);
    }
}

void USFLoadingScreenSubsystem::SetHardLoadingScreenContentWidget(TSubclassOf<UUserWidget> NewWidgetClass)
{
    if (HardLoadingScreenWidgetClass != NewWidgetClass)
    {
        HardLoadingScreenWidgetClass = NewWidgetClass;
        OnHardLoadingScreenWidgetChanged.Broadcast(HardLoadingScreenWidgetClass);
    }
}

TSubclassOf<UUserWidget> USFLoadingScreenSubsystem::GetHardLoadingScreenContentWidget() const
{
    return HardLoadingScreenWidgetClass;
}

void USFLoadingScreenSubsystem::SetSeamlessLoadingScreenContentWidget(TSubclassOf<UUserWidget> NewWidgetClass)
{
    if (SeamlessLoadingScreenWidgetClass != NewWidgetClass)
    {
        SeamlessLoadingScreenWidgetClass = NewWidgetClass;
        OnSeamlessLoadingScreenWidgetChanged.Broadcast(SeamlessLoadingScreenWidgetClass);
    }
}

TSubclassOf<UUserWidget> USFLoadingScreenSubsystem::GetSeamlessLoadingScreenContentWidget() const
{
    return SeamlessLoadingScreenWidgetClass;
}

void USFLoadingScreenSubsystem::OnSeamlessTravelStart(UWorld* CurrentWorld, const FString& LevelName)
{
    // 로딩 스크린 출력 관련 처리는 CommonLoadingScreen에서 전담

    // Preload된 위젯이 있고 목적지가 일치하는지 확인
    FString ShortLevelName = FPackageName::GetShortName(LevelName);
    FString ShortPendingName = FPackageName::GetShortName(PendingLevelName);
    
    if (PreloadedWidgetClass && ShortLevelName == ShortPendingName)
    {
        SetSeamlessLoadingScreenContentWidget(PreloadedWidgetClass);
        
        // 사용 완료 후 정리
        PreloadedWidgetClass = nullptr;
        PendingLevelName.Empty();
        CurrentWidgetLoadHandle.Reset();
        return;
    }
    
    // Preload 안 된 경우 - 폴백: 동기 로드
    const FSFMapLoadingConfig* Config = FindLoadingConfigForMap(LevelName);
    if (Config && !Config->SeamlessLoadingWidget.IsNull())
    {
        // 동기 로드 - 약간의 프레임 드랍 발생 가능
        TSubclassOf<UUserWidget> WidgetClass = Config->SeamlessLoadingWidget.LoadSynchronous();
        if (WidgetClass)
        {
            SetSeamlessLoadingScreenContentWidget(WidgetClass);
            UE_LOG(LogSF, Warning, TEXT("[SeamlessTravel] Loaded widget synchronously: %s"), *GetNameSafe(WidgetClass));
        }
    }
    else
    {
        UE_LOG(LogSF, Error, TEXT("[SeamlessTravel] No config found for: %s"), *LevelName);
    }
}

void USFLoadingScreenSubsystem::OnPreLoadMap(const FString& MapName)
{
    // Hard Travel 전에 위젯 Preload 시도
    const FSFMapLoadingConfig* Config = FindLoadingConfigForMap(MapName);
    
    if (Config && !Config->HardLoadingWidget.IsNull())
    {
        // Config는 있지만 아직 Preload 안 된 경우 
        FString ShortMapName = FPackageName::GetShortName(MapName);
        FString ShortPendingName = FPackageName::GetShortName(PendingLevelName);
        
        if (ShortMapName != ShortPendingName || !PreloadedWidgetClass)
        {
            UE_LOG(LogSF, Warning, TEXT("[HardTravel] Widget not preloaded, loading synchronously"));
            
            // 동기 로드
            TSubclassOf<UUserWidget> WidgetClass = Config->HardLoadingWidget.LoadSynchronous();
            PreloadedWidgetClass = WidgetClass;
            PendingLevelName = MapName;
        }
    }
    
    StartHardLoadingScreen();
}

void USFLoadingScreenSubsystem::StartHardLoadingScreen()
{
    if (bCurrentLoadingScreenStarted)
    {
        return;
    }
    
    if (GetMoviePlayer()->IsMovieCurrentlyPlaying())
    {
        return; 
    }

    FLoadingScreenAttributes LoadingScreen;
    UGameInstance* LocalGameInstance = GetGameInstance();
    TSubclassOf<UUserWidget> LoadedWidgetClass = HardLoadingScreenWidget.TryLoadClass<UUserWidget>();
    
    // 표시할 콘텐츠 위젯 준비 (Slate Widget 포인터 확보)
    TSharedPtr<SWidget> ContentWidget;
    if (UUserWidget* LoadingWidget = UUserWidget::CreateWidgetInstance(*LocalGameInstance, LoadedWidgetClass, NAME_None))
    {
        HardLoadingSWidgetPtr = LoadingWidget->TakeWidget();
        ContentWidget = HardLoadingSWidgetPtr;
    }
    else
    {
        // 위젯 생성 실패 시 기본 로딩 화면 사용
        ContentWidget = SNew(SSFSimpleLoadingScreen); 
    }

    // 현재 뷰포트 해상도에 따른 DPI Scale 값 계산
    float CalculatedDPIScale = 1.0f;
    if (GEngine && GEngine->GameViewport)
    {
        FVector2D ViewportSize;
        GEngine->GameViewport->GetViewportSize(ViewportSize);
        
        // 프로젝트 세팅의 DPI 커브를 기반으로 올바른 스케일 값을 가져옴 
        CalculatedDPIScale = GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(FIntPoint(ViewportSize.X, ViewportSize.Y));
    }

    // DPI Scale 적용된 콘텐츠 위젯 생성
    if (ContentWidget.IsValid())
    {
        LoadingScreen.WidgetLoadingScreen = SNew(SOverlay)
        + SOverlay::Slot() // 슬롯 추가
       .HAlign(HAlign_Fill)
       .VAlign(VAlign_Fill)
        [   // 슬롯 내용 시작
            SNew(SSafeZone) // 모바일/콘솔 안전 구역 확보
           .HAlign(HAlign_Fill)
           .VAlign(VAlign_Fill)
           .IsTitleSafe(true)
            [   // SafeZone 자식 시작
                SNew(SDPIScaler) // DPI 스케일링 적용
               .DPIScale(CalculatedDPIScale)
                [   // DPIScaler 자식 시작
                    ContentWidget.ToSharedRef() // 실제 콘텐츠 위젯
                ]
            ]
        ];
    }

    // 만약 없으면 HostLoadingScreen(부모 위젯)의 기본 검은 화면이 보일 것
    if (PreloadedWidgetClass)
    {
        SetHardLoadingScreenContentWidget(PreloadedWidgetClass);
    }
    
    // 하드 트래블 최적화
    LoadingScreen.bAllowEngineTick = false; 
    LoadingScreen.bWaitForManualStop = false;
    LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
    LoadingScreen.bMoviesAreSkippable = false;
    LoadingScreen.MinimumLoadingScreenDisplayTime = 3.0f; // 최소 로딩 스크린 표시 시간
    LoadingScreen.PlaybackType = MT_Normal;

    GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
    GetMoviePlayer()->PlayMovie();
    
    // 3D 렌더링 비활성화 (플리커링 방지)
    if (UGameViewportClient* GameViewport = GetWorld()->GetGameViewport())
    {
       GameViewport->bDisableWorldRendering = true;
    }

    bCurrentLoadingScreenStarted = true;
}

void USFLoadingScreenSubsystem::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
    if (bCurrentLoadingScreenStarted)
    {
        // 하드 트래블 완료 (bAutoCompleteWhenLoadingCompletes=true라면 엔진이 알아서 끄지만 명시적으로 호출)
        StopHardLoadingScreen();
    }
}

void USFLoadingScreenSubsystem::StopHardLoadingScreen()
{
    GetMoviePlayer()->StopMovie();
    
    if (UGameViewportClient* GameViewport = GetWorld()->GetGameViewport())
    {
        GameViewport->bDisableWorldRendering = false;
    }
    
    RemoveWidgetFromViewport();
    bCurrentLoadingScreenStarted = false;
}

void USFLoadingScreenSubsystem::RemoveWidgetFromViewport()
{
    UGameInstance* LocalGameInstance = GetGameInstance();
    if (HardLoadingSWidgetPtr.IsValid())
    {
        if (UGameViewportClient* GameViewportClient = LocalGameInstance->GetGameViewportClient())
        {
            GameViewportClient->RemoveViewportWidgetContent(HardLoadingSWidgetPtr.ToSharedRef());
        }
        HardLoadingSWidgetPtr.Reset();
    }
}

const FSFMapLoadingConfig* USFLoadingScreenSubsystem::FindLoadingConfigForMap(const FString& MapName)
{
    UDataTable* ConfigTable = LoadingConfigTable.LoadSynchronous();
    if (!ConfigTable)
    {
        return nullptr;
    }
    
    // 맵 이름에서 경로만 제거 (짧은 이름만 추출)
    FString ShortMapName = FPackageName::GetShortName(MapName);

    // DataTable에서 정확히 일치하는 Row 찾기
    FSFMapLoadingConfig* Config = ConfigTable->FindRow<FSFMapLoadingConfig>(
        FName(*ShortMapName), 
        TEXT("FindLoadingConfig")
    );
    
    if (Config)
    {
        return Config;
    }

    return nullptr;
}

void USFLoadingScreenSubsystem::OnConfigTableLoaded()
{
    CachedConfigTable = LoadingConfigTable.Get();
    
    if (CachedConfigTable)
    {
        UE_LOG(LogSF, Log, TEXT("[LoadingScreenSubsystem] Config table loaded successfully"));
    }
    else
    {
        UE_LOG(LogSF, Error, TEXT("[LoadingScreenSubsystem] Failed to load config table!"));
    }
    
    ConfigTableLoadHandle.Reset();
}

void USFLoadingScreenSubsystem::OnLoadingScreenAssetLoaded()
{
    if (!CurrentWidgetLoadHandle.IsValid())
    {
        return;
    }
    
    // 로드된 위젯 클래스 가져오기
    UObject* LoadedAsset = CurrentWidgetLoadHandle->GetLoadedAsset();
    
    if (UClass* WidgetClass = Cast<UClass>(LoadedAsset))
    {
        if (WidgetClass->IsChildOf(UUserWidget::StaticClass()))
        {
            PreloadedWidgetClass = TSubclassOf<UUserWidget>(WidgetClass);
        }
    }
    else
    {
        UE_LOG(LogSF, Error, TEXT("[Preload] Failed to load asset for level: %s"), *PendingLevelName);
    }
}
