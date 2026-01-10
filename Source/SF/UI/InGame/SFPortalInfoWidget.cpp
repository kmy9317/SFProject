#include "SFPortalInfoWidget.h"

#include "SFLogChannels.h"
#include "SFPortalInfoEntryWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "GameModes/SFGameState.h"
#include "Messages/SFMessageGameplayTags.h"
#include "Messages/SFPortalInfoMessages.h"
#include "Player/SFPlayerState.h"

void USFPortalInfoWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 초기 상태는 숨김
    SetVisibility(ESlateVisibility::Collapsed);
    Text_Countdown->SetVisibility(ESlateVisibility::Collapsed);

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    
    // GameState 델리게이트 바인딩
    ASFGameState* SFGameState = World->GetGameState<ASFGameState>();
    if (SFGameState)
    {
        // 델리게이트에 함수 바인딩
        SFGameState->OnPlayerAdded.AddDynamic(this, &USFPortalInfoWidget::HandlePlayerAdded);
        SFGameState->OnPlayerRemoved.AddDynamic(this, &USFPortalInfoWidget::HandlePlayerRemoved);

        // 초기 플레이어들 추가 (순서는 나중에 OnRep으로 정렬됨)
        for (APlayerState* PS : SFGameState->PlayerArray)
        {
            HandlePlayerAdded(PS);
        }
    }

    // GMS 리스너 등록
    UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(World);
    
    // Portal Manager의 글로벌 PortalState 변경 메시지 리스너로 등록
    PortalInfoListenerHandle = MessageSubsystem.RegisterListener(
        SFGameplayTags::Message_Portal_StateChanged, 
        this, 
        &USFPortalInfoWidget::HandlePortalInfoChanged);

    // 플레이어별 준비 상태 변경에 대한 리스너로 등록
    PlayerReadyListenerHandle = MessageSubsystem.RegisterListener(
        SFGameplayTags::Message_Player_TravelReadyChanged, 
        this, 
        &USFPortalInfoWidget::HandlePlayerReadyChanged);

    // Dead 상태 변경 리스너 등록
    DeadStateListenerHandle = MessageSubsystem.RegisterListener(
        SFGameplayTags::Message_Player_DeadStateChangedUI,
        this,
        &USFPortalInfoWidget::HandlePlayerDeadStateChanged);
}

void USFPortalInfoWidget::NativeDestruct()
{
    // 리스너 및 델리게이트 정리
    if (UWorld* World = GetWorld())
    {
        if (ASFGameState* SFGameState = World->GetGameState<ASFGameState>())
        {
            SFGameState->OnPlayerAdded.RemoveAll(this);
            SFGameState->OnPlayerRemoved.RemoveAll(this);
        }
        if (UGameplayMessageSubsystem::HasInstance(this))
        {
            UGameplayMessageSubsystem& GMS = UGameplayMessageSubsystem::Get(World);
            GMS.UnregisterListener(PortalInfoListenerHandle);
            GMS.UnregisterListener(PlayerReadyListenerHandle);
            GMS.UnregisterListener(DeadStateListenerHandle);
        }
    }
    
    Super::NativeDestruct();
}

void USFPortalInfoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bIsCountingDown)
    {
        CurrentCountdownTime -= InDeltaTime;
        if (CurrentCountdownTime <= 0.0f)
        {
            CurrentCountdownTime = 0.0f;
        }

        // 텍스트 갱신
        UpdateCountdownText();
    }
}

void USFPortalInfoWidget::HandlePlayerAdded(APlayerState* PlayerState)
{
    if (!PlayerState || PortalEntryMap.Contains(PlayerState))
    {
        return;
    }

    if (!PortalEntryClass)
    {
        UE_LOG(LogSF, Warning, TEXT("USFPortalInfoWidget: PortalEntryClass is not set."));
        return;
    }

    ASFPlayerState* SFPlayerState = Cast<ASFPlayerState>(PlayerState);
    if (!SFPlayerState)
    {
        return;
    }

    USFPortalInfoEntryWidget* NewPortalEntry = CreateWidget<USFPortalInfoEntryWidget>(this, PortalEntryClass);
    if (NewPortalEntry)
    {
        // Entry가 자체적으로 PlayerState에 바인딩 (정보 갱신용)
        NewPortalEntry->InitializeRow(SFPlayerState);
        
        PortalEntryBox->AddChild(NewPortalEntry);
        PortalEntryMap.Add(PlayerState, NewPortalEntry);

        // 정렬용 바인딩 (Entry의 바인딩과 별개)
        SFPlayerState->OnPlayerInfoChanged.AddDynamic(this, &USFPortalInfoWidget::HandlePlayerInfoChangedForReorder);

        // Add 시점에 즉시 정렬
        ReorderAllEntries();
    }
}

void USFPortalInfoWidget::HandlePlayerRemoved(APlayerState* PlayerState)
{
    if (!PlayerState)
    {
        return;
    }

    // 델리게이트 해제
    if (ASFPlayerState* SFPS = Cast<ASFPlayerState>(PlayerState))
    {
        SFPS->OnPlayerInfoChanged.RemoveAll(this);
    }

    USFPortalInfoEntryWidget* EntryToRemove = nullptr;
    TWeakObjectPtr<APlayerState> KeyToRemove;
    
    for (auto& Pair : PortalEntryMap)
    {
        if (Pair.Key.Get() == PlayerState)
        {
            EntryToRemove = Pair.Value;
            KeyToRemove = Pair.Key;
            break;
        }
    }
    
    if (EntryToRemove)
    {
        EntryToRemove->RemoveFromParent();
        PortalEntryMap.Remove(KeyToRemove);
    }
}

void USFPortalInfoWidget::HandlePortalInfoChanged(FGameplayTag Channel, const FSFPortalStateMessage& Message)
{
    // Message.bIsActive는 최초로 포탈로 이동한 플레이어가 존재하여 UI를 표시해야 함을 의미함. 
    if (Message.bIsActive)
    {
        SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        SetVisibility(ESlateVisibility::Collapsed);
    }

    // 맵 이동 카운트 다운 텍스트 표시
    if (Message.TravelCountdown > 0.0f)
    {
        bIsCountingDown = true;

        // TODO : 서버에서 클라로 메시지 도착 시간에 따른 카운트다운 시간 조정 필요
        CurrentCountdownTime = Message.TravelCountdown;
        Text_Countdown->SetVisibility(ESlateVisibility::Visible);
        UpdateCountdownText();
    }
    else
    {
        bIsCountingDown = false;
        Text_Countdown->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void USFPortalInfoWidget::HandlePlayerReadyChanged(FGameplayTag Channel, const FSFPlayerTravelReadyMessage& Message)
{
    if (!Message.PlayerState)
    {
        return;
    }

    // TMap에서 해당 플레이어의 Entry 위젯 찾기
    if (USFPortalInfoEntryWidget* EntryToUpdate = PortalEntryMap.FindRef(Message.PlayerState))
    {
        // Entry 위젯 내부의 함수를 호출하여 '준비' UI만 업데이트
        EntryToUpdate->SetReadyStatus(Message.bIsReadyToTravel);
    }
}

void USFPortalInfoWidget::HandlePlayerDeadStateChanged(FGameplayTag Channel, const FSFPlayerDeadStateMessage& Message)
{
    if (USFPortalInfoEntryWidget* Entry = PortalEntryMap.FindRef(Message.PlayerState))
    {
        Entry->SetDeadStatus(Message.bIsDead);
    }
}

void USFPortalInfoWidget::HandlePlayerInfoChangedForReorder(const FSFPlayerSelectionInfo& NewPlayerSelection)
{
    ReorderAllEntries();
}

void USFPortalInfoWidget::UpdateCountdownText()
{
    if (Text_Countdown)
    {
        int32 DisplayTime = FMath::CeilToInt(CurrentCountdownTime);
        Text_Countdown->SetText(FText::AsNumber(DisplayTime));
    }
}

void USFPortalInfoWidget::ReorderAllEntries()
{
    TArray<TPair<uint8, USFPortalInfoEntryWidget*>> SortedEntries;
    
    for (auto& Pair : PortalEntryMap)
    {
        if (ASFPlayerState* SFPS = Cast<ASFPlayerState>(Pair.Key.Get()))
        {
            uint8 SlotId = SFPS->GetPlayerSelection().GetPlayerSlot();
            SortedEntries.Add(TPair<uint8, USFPortalInfoEntryWidget*>(SlotId, Pair.Value));
        }
    }

    SortedEntries.Sort([](const auto& A, const auto& B)
    {
        return A.Key < B.Key;
    });

    PortalEntryBox->ClearChildren();

    
    
    for (auto& Entry : SortedEntries)
    {
        PortalEntryBox->AddChild(Entry.Value);
    }
}
