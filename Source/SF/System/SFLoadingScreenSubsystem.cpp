#include "SFLoadingScreenSubsystem.h"
#include "MoviePlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "GameMapsSettings.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameViewportClient.h"
#include "UI/Slate/SFSimpleLoadingScreen.h"

#include "HAL/IConsoleManager.h"

void USFLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
    
    // 1. 하드 트래블(OpenLevel) 감지
    FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &USFLoadingScreenSubsystem::OnPreLoadMap);

    // 2. 맵 로드 완료 감지 (하드/심리스 공통)
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USFLoadingScreenSubsystem::OnPostLoadMapWithWorld);

    // 3. 심리스 트래블(ServerTravel) 시작 감지 TODO : CommonLoadingScreen 시스템에서 이를 대체할 예정
    //FWorldDelegates::OnSeamlessTravelStart.AddUObject(this, &USFLoadingScreenSubsystem::OnSeamlessTravelStart);
    
    bIsSeamlessTravelInProgress = false;
    bCurrentLoadingScreenStarted = false;
}

void USFLoadingScreenSubsystem::Deinitialize()
{
    RemoveWidgetFromViewport();
    
    FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
    FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
    FWorldDelegates::OnSeamlessTravelStart.RemoveAll(this);
	Super::Deinitialize();
}

bool USFLoadingScreenSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UGameInstance* GameInstance = CastChecked<UGameInstance>(Outer);
    const bool bIsServerWorld = GameInstance->IsDedicatedServerInstance();
    return !bIsServerWorld;
}

void USFLoadingScreenSubsystem::OnPreLoadMap(const FString& MapName)
{
    bIsSeamlessTravelInProgress = false;
    StartLoadingScreen();
    
}

void USFLoadingScreenSubsystem::OnSeamlessTravelStart(UWorld* CurrentWorld, const FString& LevelName)
{
    bIsSeamlessTravelInProgress = true;
    StartLoadingScreen();
}

void USFLoadingScreenSubsystem::StartLoadingScreen()
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
    TSubclassOf<UUserWidget> LoadingScreenWidgetClass = LoadingScreenWidget.TryLoadClass<UUserWidget>();
    if (UUserWidget* LoadingWidget = UUserWidget::CreateWidgetInstance(*LocalGameInstance, LoadingScreenWidgetClass, NAME_None))
    {
        LoadingSWidgetPtr = LoadingWidget->TakeWidget();
        LoadingScreen.WidgetLoadingScreen = LoadingSWidgetPtr;
    }
    else
    {
        // 기본적으로 정의한 SSimpleLoadingScreen 사용
        LoadingScreen.WidgetLoadingScreen = SNew(SSFSimpleLoadingScreen); 
    }
    
    if (bIsSeamlessTravelInProgress)
    {
        LoadingScreen.bAllowEngineTick = false;   
        LoadingScreen.bWaitForManualStop = true; 
        LoadingScreen.bAutoCompleteWhenLoadingCompletes = false;
    }
    else
    {
        // 하드 트래블 최적화
        LoadingScreen.bAllowEngineTick = false; 
        LoadingScreen.bWaitForManualStop = false;
        LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
    }

    LoadingScreen.bMoviesAreSkippable = false;
    LoadingScreen.MinimumLoadingScreenDisplayTime = 3.0f; // 최소 노출 시간
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
    FString LoadedMapName = LoadedWorld->GetPackage()->GetName();
    if (bIsSeamlessTravelInProgress)
    {
        if (IsInSeamlessTravel())
        {
            return; 
        }
        else
        {
            if (bCurrentLoadingScreenStarted)
            {
                //StopLoadingScreen();
            }
        }
    }
    else
    {
        if (bCurrentLoadingScreenStarted)
        {
            // 하드 트래블 완료 (bAutoCompleteWhenLoadingCompletes=true라면 엔진이 알아서 끄지만 명시적으로 호출)
            StopLoadingScreen();
        }
    }
}

void USFLoadingScreenSubsystem::StopLoadingScreen()
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
    if (LoadingSWidgetPtr.IsValid())
    {
        if (UGameViewportClient* GameViewportClient = LocalGameInstance->GetGameViewportClient())
        {
            GameViewportClient->RemoveViewportWidgetContent(LoadingSWidgetPtr.ToSharedRef());
        }
        LoadingSWidgetPtr.Reset();
    }
}

bool USFLoadingScreenSubsystem::IsInSeamlessTravel()
{
    if (UWorld* World = GetWorld())
    {
        if (GEngine)
        {
            // 현재 월드 컨텍스트의 트래블 핸들러 상태 확인
            FSeamlessTravelHandler& TravelHandler = GEngine->SeamlessTravelHandlerForWorld(World);
            return TravelHandler.IsInTransition();
        }
    }
    return false;
}

bool USFLoadingScreenSubsystem::IsTransitionMap(const FString& MapName)
{
    if (UGameMapsSettings* GameMapsSettings = UGameMapsSettings::GetGameMapsSettings())
    {
        // 프로젝트 세팅에 설정된 트랜지션 맵 이름과 비교
        const FString TransitionMapName = UGameMapsSettings::GetGameMapsSettings()->TransitionMap.GetLongPackageName();
    
        // 경로 포함 비교 또는 이름만 비교
        return MapName.Contains(TransitionMapName) || TransitionMapName.Contains(MapName);
    }
    return false;
}