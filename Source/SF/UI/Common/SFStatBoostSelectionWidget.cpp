#include "SFStatBoostSelectionWidget.h"

#include "SFStatBoostCardWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"
#include "System/Data/Common/SFCommonUpgradeChoice.h"
#include "Player/SFPlayerState.h"
#include "Player/Components/SFCommonUpgradeComponent.h"

void USFStatBoostSelectionWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    CardWidgets = { Card_01, Card_02, Card_03 };

    for (USFStatBoostCardWidget* Card : CardWidgets)
    {
        if (Card)
        {
            Card->OnCardSelectedDelegate.AddDynamic(this, &ThisClass::OnCardSelected);
            Card->OnAnimationFinishedDelegate.AddDynamic(this, &ThisClass::OnCardAnimationFinished);
        }
    }

    if (Btn_Reroll)
    {
        Btn_Reroll->OnClicked.AddDynamic(this, &ThisClass::OnRerollClicked);
    }

    // 추가 보너스 선택 알림 숨김
    if (Text_ExtraSelectionNotice)
    {
        Text_ExtraSelectionNotice->SetVisibility(ESlateVisibility::Collapsed);
    }

    UpdateRerollButtonState();
}

void USFStatBoostSelectionWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void USFStatBoostSelectionWidget::NativeDestruct()
{
    for (USFStatBoostCardWidget* Card : CardWidgets)
    {
        if (Card)
        {
            Card->OnCardSelectedDelegate.RemoveAll(this);
            Card->OnAnimationFinishedDelegate.RemoveAll(this);
        }
    }

    if (Btn_Reroll)
    {
        Btn_Reroll->OnClicked.RemoveAll(this);
    }

    Super::NativeDestruct();
}

void USFStatBoostSelectionWidget::InitializeWithChoices(const TArray<FSFCommonUpgradeChoice>& Choices, int32 NextRerollCost)
{
    CachedNextRerollCost = NextRerollCost;
    SetChoices(Choices);
}

void USFStatBoostSelectionWidget::RefreshChoices(const TArray<FSFCommonUpgradeChoice>& Choices, int32 NextRerollCost)
{
    if (bAnimationPlaying)
    {
        PendingAction = EPendingAction::Refresh;
        PendingChoices = Choices;
        PendingNextRerollCost = NextRerollCost; 
        return;
    }
    
    CachedNextRerollCost = NextRerollCost;
    SetChoices(Choices);
    BP_OnChoicesRefreshed(); // TODO : 선택지 갱신시(리롤/추가 보너스) 애니메이션 Play 가능
}

void USFStatBoostSelectionWidget::RefreshChoicesWithExtraNotice(const TArray<FSFCommonUpgradeChoice>& Choices, int32 NextRerollCost)
{
    if (bAnimationPlaying)
    {
        PendingAction = EPendingAction::RefreshWithNotice;
        PendingChoices = Choices;
        PendingNextRerollCost = NextRerollCost;
        return;
    }

    CachedNextRerollCost = NextRerollCost;
    ShowExtraSelectionNotice();
    SetChoices(Choices);
    BP_OnChoicesRefreshed();
}

void USFStatBoostSelectionWidget::SetChoices(const TArray<FSFCommonUpgradeChoice>& Choices)
{
    for (int32 i = 0; i < CardWidgets.Num(); ++i)
    {
        if (CardWidgets[i] && Choices.IsValidIndex(i))
        {
            CardWidgets[i]->SetCardData(Choices[i], i);
            CardWidgets[i]->SetVisibility(ESlateVisibility::Visible);
            
            // 순차 등장 애니메이션 (0.2초 간격)
            float Delay = i * CardRevealDelay;
            if (Delay > 0.0f)
            {
                FTimerHandle TempHandle;
                GetWorld()->GetTimerManager().SetTimer(
                    TempHandle,
                    FTimerDelegate::CreateWeakLambda(CardWidgets[i].Get(), [Card = CardWidgets[i].Get()]()
                    {
                        Card->PlayCardReveal();
                    }),
                    Delay,
                    false
                );
            }
            else
            {
                // 첫 번째 카드는 바로 실행
                CardWidgets[i]->PlayCardReveal();
            }
        }
    }
    
    EnableRerollButtonWithDelay();
}

void USFStatBoostSelectionWidget::OnCardSelected(int32 CardIndex)
{
    // 애니메이션 시작
    bAnimationPlaying = true;
    PendingAction = EPendingAction::None;
    PendingChoices.Empty();
    
    if (Text_ExtraSelectionNotice)
    {
        Text_ExtraSelectionNotice->SetVisibility(ESlateVisibility::Collapsed);
    }
    
    DisableAllCards();

    for (int32 i = 0; i < CardWidgets.Num(); ++i)
    {
        if (CardWidgets[i])
        {
            if (i != CardIndex)
            {
                CardWidgets[i]->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }

    // 리롤 버튼도 비활성화
    if (Btn_Reroll)
    {
        Btn_Reroll->SetIsEnabled(false);
    }

    OnCardSelectedDelegate.Broadcast(CardIndex);
}

void USFStatBoostSelectionWidget::OnCardAnimationFinished()
{
    bAnimationPlaying = false;
    ProcessPendingAction();
}

void USFStatBoostSelectionWidget::OnRerollClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        return;
    }

    ASFPlayerState* PS = PC->GetPlayerState<ASFPlayerState>();
    if (!PS)
    {
        return;
    }

    USFCommonUpgradeComponent* UpgradeComp = PS->FindComponentByClass<USFCommonUpgradeComponent>();
    if (UpgradeComp)
    {
        UpgradeComp->RequestReroll();
    }
    if (Btn_Reroll)
    {
        Btn_Reroll->SetIsEnabled(false);
    }

    DisableAllCards();
}

void USFStatBoostSelectionWidget::EnableRerollButtonWithDelay()
{
    if (!Btn_Reroll)
    {
        return;
    }

    Btn_Reroll->SetIsEnabled(false);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RerollButtonEnableTimerHandle);

        World->GetTimerManager().SetTimer(
            RerollButtonEnableTimerHandle,
            FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                UpdateRerollButtonState();
            }),
            RerollButtonEnableDelay,  // 카드와 동일한 딜레이
            false
        );
    }
}

void USFStatBoostSelectionWidget::UpdateRerollButtonState()
{
    if (!Btn_Reroll)
    {
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        Btn_Reroll->SetIsEnabled(false);
        return;
    }

    ASFPlayerState* PS = PC->GetPlayerState<ASFPlayerState>();
    if (!PS)
    {
        Btn_Reroll->SetIsEnabled(false);
        return;
    }

    bool bCanAfford = (CachedNextRerollCost == 0) || (PS->GetGold() >= CachedNextRerollCost);
    Btn_Reroll->SetIsEnabled(bCanAfford);
    UpdateRerollCostDisplay(CachedNextRerollCost);
}

void USFStatBoostSelectionWidget::UpdateRerollCostDisplay(int32 Cost)
{
    if (!Text_RerollCost)
    {
        return;
    }

    if (Cost == 0)
    {
        Text_RerollCost->SetText(NSLOCTEXT("SF", "FreeReroll", "무료"));
    }
    else
    {
        Text_RerollCost->SetText(FText::Format(NSLOCTEXT("SF", "RerollCostFormat", "{0} G"), FText::AsNumber(Cost)));
    }
}

void USFStatBoostSelectionWidget::DisableAllCards()
{
    for (USFStatBoostCardWidget* Card : CardWidgets)
    {
        if (Card)
        {
            Card->SetButtonEnabled(false);
        }
    }
}

void USFStatBoostSelectionWidget::ShowExtraSelectionNotice()
{
    if (Text_ExtraSelectionNotice)
    {
        Text_ExtraSelectionNotice->SetVisibility(ESlateVisibility::Visible);

        if (Anim_ExtraNotice)
        {
            PlayAnimation(Anim_ExtraNotice);
        }
    }

    BP_ShowExtraSelectionNotice();
}

void USFStatBoostSelectionWidget::ShowErrorMessage(const FText& Message)
{
    // TODO : 에러용인데 사용 안해도 됨
    BP_ShowErrorMessage(Message);
}

void USFStatBoostSelectionWidget::ProcessPendingAction()
{
    switch (PendingAction)
    {
    case EPendingAction::Refresh:
        RefreshChoices(PendingChoices, PendingNextRerollCost);
        PendingChoices.Empty();
        break;
    case EPendingAction::RefreshWithNotice:
        RefreshChoicesWithExtraNotice(PendingChoices, PendingNextRerollCost);
        PendingChoices.Empty();
        break;
    case EPendingAction::Close:
        OnSelectionCompleteDelegate.Broadcast();
        break;
    case EPendingAction::None:
    default:
        break;
    }
    PendingAction = EPendingAction::None;
}

void USFStatBoostSelectionWidget::NotifyClose()
{
    if (bAnimationPlaying)
    {
        // 애니메이션 중이면 대기
        PendingAction = EPendingAction::Close;
        return;
    }

    OnSelectionCompleteDelegate.Broadcast();
}