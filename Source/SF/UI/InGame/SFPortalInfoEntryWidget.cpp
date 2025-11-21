#include "SFPortalInfoEntryWidget.h"

#include "Player/SFPlayerState.h" 
#include "Character/Hero/SFHeroDefinition.h" 
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "System/SFAssetManager.h" 

void USFPortalInfoEntryWidget::InitializeRow(ASFPlayerState* OwningPlayerState)
{
    if (!OwningPlayerState)
    {
        return;
    }

    // PlayerState의 bIsReadyForTravel 값을 읽어와 UI에 즉시 반영
    SetReadyStatus(OwningPlayerState->GetIsReadyForTravel());
    
    const FSFPlayerSelectionInfo& SelectionInfo = OwningPlayerState->GetPlayerSelection();

    // PlayerState의 PlayerInfo를 읽어와 UI에 즉시 반영
    HandlePlayerInfoChanged(SelectionInfo);

    // Replicated PlayerInfo 변경 시 UI 업데이트를 위한 델리게이트 등록
    OwningPlayerState->OnPlayerInfoChanged.AddDynamic(this, &USFPortalInfoEntryWidget::HandlePlayerInfoChanged);
}

void USFPortalInfoEntryWidget::SetReadyStatus(bool bIsReady)
{
    if (Img_ReadyCheck)
    {
        Img_ReadyCheck->SetVisibility(bIsReady? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
    }
}

void USFPortalInfoEntryWidget::NativeDestruct()
{
    // 위젯이 파괴될 때 (스테이지 이동 or 플레이어가 중간에 이탈)
    // 아직 로드 중인 아이콘이 있다면 로드를 취소 (크래시 방지)
    if (IconLoadHandle.IsValid())
    {
        IconLoadHandle->CancelHandle();
        IconLoadHandle.Reset();
    }

    Super::NativeDestruct();
}

void USFPortalInfoEntryWidget::HandlePlayerInfoChanged(const FSFPlayerSelectionInfo& NewPlayerSelection)
{
    if (Text_PlayerName)
    {
        Text_PlayerName->SetText(FText::FromString(NewPlayerSelection.GetPlayerNickname()));
    }
    
    if (USFHeroDefinition* HeroDef = NewPlayerSelection.GetHeroDefinition())
    {
        const TSoftObjectPtr<UTexture2D> IconPath = HeroDef->GetIconPath(); 

        if (IconPath.IsNull())
        {
            if (Img_HeroIcon)
            {
                Img_HeroIcon->SetBrush(FSlateBrush());
                return;
            }
        }

        // Hero 아이콘 비동기 로드
        FStreamableManager& Streamable = USFAssetManager::Get().GetStreamableManager();
        IconLoadHandle = Streamable.RequestAsyncLoad(IconPath.ToSoftObjectPath(),
            FStreamableDelegate::CreateUObject(this, &USFPortalInfoEntryWidget::OnIconLoadCompleted));
    }
}

void USFPortalInfoEntryWidget::OnIconLoadCompleted()
{
    if (IconLoadHandle.IsValid() && Img_HeroIcon)
    {
        UTexture2D* LoadedIcon = Cast<UTexture2D>(IconLoadHandle->GetLoadedAsset());
        if (LoadedIcon)
        {
            Img_HeroIcon->SetBrushFromTexture(LoadedIcon);
        }
    }

    IconLoadHandle.Reset();
}