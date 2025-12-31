#include "SFStatBoostCardWidget.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "System/Data/Common/SFCommonUpgradeChoice.h"
#include "System/Data/Common/SFCommonUpgradeDefinition.h"
#include "System/Data/Common/SFCommonRarityConfig.h"

void USFStatBoostCardWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Select)
    {
        Btn_Select->OnClicked.AddDynamic(this, &ThisClass::OnCardClicked);
        Btn_Select->SetIsEnabled(false);
    }
}

void USFStatBoostCardWidget::NativeDestruct()
{
    if (Btn_Select)
    {
        Btn_Select->OnClicked.RemoveAll(this);
    }
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ButtonEnableTimerHandle);
    }

    Super::NativeDestruct();
}

void USFStatBoostCardWidget::SetCardData(const FSFCommonUpgradeChoice& Choice, int32 InCardIndex)
{
    CurrentCardIndex = InCardIndex;

    const USFCommonUpgradeDefinition* UpgradeDef = Choice.UpgradeDefinition;
    if (!UpgradeDef)
    {
        return;
    }

    // 제목
    if (Text_Title)
    {
        Text_Title->SetText(UpgradeDef->DisplayName);
    }

    // 설명 (동적 수치가 추가된 텍스트 사용)
    if (Text_Desc)
    {
        Text_Desc->SetText(Choice.DynamicDescription);
    }

    // 아이콘
    if (Image_Icon && !UpgradeDef->Icon.IsNull())
    {
        UTexture2D* IconTexture = UpgradeDef->Icon.LoadSynchronous();
        if (IconTexture)
        {
            Image_Icon->SetBrushFromTexture(IconTexture);
        }
    }

    // 등급 비주얼
    ApplyRarityVisuals(Choice.RarityConfig);

    // 데이터 설정 시 딜레이 활성화
    EnableButtonWithDelay();
}

void USFStatBoostCardWidget::EnableButtonWithDelay()
{
    if (Btn_Select)
    {
        Btn_Select->SetIsEnabled(false);
    }

    if (UWorld* World = GetWorld())
    {
        // 기존 타이머 클리어
        World->GetTimerManager().ClearTimer(ButtonEnableTimerHandle);

        World->GetTimerManager().SetTimer(
            ButtonEnableTimerHandle,
            FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                SetButtonEnabled(true);
            }),
            ButtonEnableDelay,
            false
        );
    }
}

void USFStatBoostCardWidget::SetButtonEnabled(bool bEnabled)
{
    if (Btn_Select)
    {
        Btn_Select->SetIsEnabled(bEnabled);
    }
}

void USFStatBoostCardWidget::ApplyRarityVisuals(const USFCommonRarityConfig* RarityConfig)
{
    if (!RarityConfig)
    {
        return;
    }

    // 등급 프레임 텍스처
    if (Image_RarityFrame && !RarityConfig->FrameTexture.IsNull())
    {
        UTexture2D* FrameTexture = RarityConfig->FrameTexture.LoadSynchronous();
        if (FrameTexture)
        {
            Image_RarityFrame->SetBrushFromTexture(FrameTexture);
        }
    }

    // 등급 색상
    if (Border_Background)
    {
        Border_Background->SetBrushColor(RarityConfig->FrameColor);
    }
}

void USFStatBoostCardWidget::OnCardClicked()
{
    SetButtonEnabled(false);
    OnCardSelectedDelegate.Broadcast(CurrentCardIndex);
}

void USFStatBoostCardWidget::NotifyAnimationComplete()
{
    OnAnimationFinishedDelegate.Broadcast();
}