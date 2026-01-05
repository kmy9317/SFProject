#include "SFGameOverStatsWidget.h"

#include "SFPlayerStatsEntryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Messages/SFMessageGameplayTags.h"
#include "Messages/SFPortalInfoMessages.h"
#include "Player/SFPlayerController.h"
#include "Player/SFPlayerInfoTypes.h"

USFGameOverStatsWidget::USFGameOverStatsWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{

}

void USFGameOverStatsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetVisibility(ESlateVisibility::Collapsed);

    if (UGameplayMessageSubsystem::HasInstance(this))
    {
        UGameplayMessageSubsystem& GMS = UGameplayMessageSubsystem::Get(this);
        StatsListenerHandle = GMS.RegisterListener(SFGameplayTags::Message_Game_GameOverStats, this, &ThisClass::OnGameOverStatsReceived);
        ReadyCountListenerHandle = GMS.RegisterListener(SFGameplayTags::Message_Game_LobbyReadyCount,this,&ThisClass::OnLobbyReadyCountReceived);
    }

    if (ReadyButton)
    {
        ReadyButton->OnClicked.AddDynamic(this, &ThisClass::OnReadyButtonClicked);
    }
}

void USFGameOverStatsWidget::NativeDestruct()
{
    if (UGameplayMessageSubsystem::HasInstance(this))
    {
        UGameplayMessageSubsystem& GMS = UGameplayMessageSubsystem::Get(this);
        GMS.UnregisterListener(StatsListenerHandle);
        GMS.UnregisterListener(ReadyCountListenerHandle);
    }

    Super::NativeDestruct();
}

void USFGameOverStatsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bCountdownActive)
    {
        UpdateCountdownDisplay();
    }
}

void USFGameOverStatsWidget::OnGameOverStatsReceived(FGameplayTag Channel, const FSFGameOverResult& Result)
{
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    InitializeStats(Result);
}

void USFGameOverStatsWidget::OnLobbyReadyCountReceived(FGameplayTag Channel, const FSFLobbyReadyMessage& Message)
{
    UpdateReadyCountDisplay(Message.ReadyCount, Message.TotalCount);
}

void USFGameOverStatsWidget::InitializeStats(const FSFGameOverResult& Result)
{
    if (StageNameText)
    {
        StageNameText->SetText(Result.StageName);
    }
    
    DisplayPlayerStats(Result.PlayerStats);

    TargetLobbyTime = Result.TargetLobbyTime;
    bCountdownActive = true;

    UpdateReadyCountDisplay(0, Result.PlayerStats.Num());
    UpdateCountdownDisplay();
}

void USFGameOverStatsWidget::DisplayPlayerStats(const TArray<FSFPlayerGameStats>& PlayerStats)
{
    if (!StatsContainer || !PlayerStatsEntryClass)
    {
        return;
    }

    StatsContainer->ClearChildren();

    for (const FSFPlayerGameStats& Stats : PlayerStats)
    {
        USFPlayerStatsEntryWidget* EntryWidget = CreateWidget<USFPlayerStatsEntryWidget>(GetOwningPlayer(), PlayerStatsEntryClass);
        if (EntryWidget)
        {
            EntryWidget->SetPlayerStats(Stats);  
            StatsContainer->AddChild(EntryWidget);
        }
    }
}

void USFGameOverStatsWidget::UpdateCountdownDisplay()
{
    float RemainingTime = GetRemainingTime();

    if (CountdownText)
    {
        // 소수점 없이 정수로 표시
        int32 SecondsRemaining = FMath::CeilToInt(RemainingTime);
        CountdownText->SetText(FText::FromString(FString::Printf(TEXT("%d초 후 로비로 이동"), SecondsRemaining)));
    }

    if (RemainingTime <= 0.f)
    {
        bCountdownActive = false;
        if (CountdownText)
        {
            CountdownText->SetText(FText::FromString(TEXT("로비로 이동 중...")));
        }
    }
}

void USFGameOverStatsWidget::UpdateReadyCountDisplay(int32 InReadyCount, int32 InTotalCount)
{
    if (ReadyCountText)
    {
        ReadyCountText->SetText(FText::FromString(FString::Printf(TEXT("준비 완료: %d / %d"), InReadyCount, InTotalCount)));
    }
}

float USFGameOverStatsWidget::GetRemainingTime() const
{
    if (const UWorld* World = GetWorld())
    {
        if (const AGameStateBase* GS = World->GetGameState<AGameStateBase>())
        {
            float CurrentServerTime = GS->GetServerWorldTimeSeconds();
            return FMath::Max(0.f, TargetLobbyTime - CurrentServerTime);
        }
    }
    return 0.f;
}

void USFGameOverStatsWidget::OnReadyButtonClicked()
{
    if (bIsReady)
    {
        return;
    }

    bIsReady = true;

    if (ReadyButton)
    {
        ReadyButton->SetIsEnabled(false);
    }

    if (ASFPlayerController* PC = Cast<ASFPlayerController>(GetOwningPlayer()))
    {
        PC->RequestReadyForLobby();
    }
}