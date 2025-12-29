#include "SFPortalInfoEntryWidget.h"

#include "SFLogChannels.h"
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

    CachedPlayerState = OwningPlayerState;

    // PlayerState의 bIsReadyForTravel 값을 읽어와 UI에 즉시 반영
    SetReadyStatus(OwningPlayerState->GetIsReadyForTravel());

    // 초기 Dead 상태 반영 (Seamless Travel 후 이미 죽은 플레이어 처리)
    SetDeadStatus(OwningPlayerState->IsDead());
    
    const FSFPlayerSelectionInfo& SelectionInfo = OwningPlayerState->GetPlayerSelection();

    // PlayerState의 PlayerInfo를 읽어와 UI에 즉시 반영
    HandlePlayerInfoChanged(SelectionInfo);

    // Replicated PlayerInfo 변경 시 UI 업데이트를 위한 델리게이트 등록
    OwningPlayerState->OnPlayerInfoChanged.AddDynamic(this, &USFPortalInfoEntryWidget::HandlePlayerInfoChanged);
    
}

void USFPortalInfoEntryWidget::SetReadyStatus(bool bIsReady)
{
    if (bIsPlayerDead)
    {
        bIsReady = false;
    }
    
    if (Img_ReadyCheck)
    {
        Img_ReadyCheck->SetVisibility(bIsReady? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
    }
}

void USFPortalInfoEntryWidget::SetDeadStatus(bool bIsDead)
{
    bIsPlayerDead = bIsDead;

    if (bIsDead)
    {
        // Ready 체크 강제 숨김
        SetReadyStatus(false);
        
        // 시각적 처리: Grayed out
        if (Img_HeroIcon)
        {
            Img_HeroIcon->SetColorAndOpacity(FLinearColor(0.3f, 0.3f, 0.3f, 0.6f));
        }
    }
    else
    {
        // 부활 시 복원
        if (Img_HeroIcon)
        {
            Img_HeroIcon->SetColorAndOpacity(FLinearColor::White);
        }
    }
}

void USFPortalInfoEntryWidget::NativeDestruct()
{
    // 위젯이 파괴될 때 (스테이지 이동 or 플레이어가 중간에 이탈)
    // 아직 로드 중인 아이콘이 있다면 로드를 취소 (크래시 방지)
    
    Super::NativeDestruct();
}

void USFPortalInfoEntryWidget::HandlePlayerInfoChanged(const FSFPlayerSelectionInfo& NewPlayerSelection)
{
    if (Text_PlayerName)
    {
        Text_PlayerName->SetText(FText::FromString(NewPlayerSelection.GetPlayerNickname()));
    }
    
    USFHeroDefinition* HeroDef = NewPlayerSelection.GetHeroDefinition();
    if (!HeroDef)
    {
        return;
    }
    
    const TSoftObjectPtr<UTexture2D> IconPath = HeroDef->GetIconPath(); 

    if (IconPath.IsNull())
    {
        if (Img_HeroIcon)
        {
            Img_HeroIcon->SetBrush(FSlateBrush());
        }
        return;
    }

    // 동기 로드
    UTexture2D* LoadedIcon = HeroDef->LoadIcon();
    if (LoadedIcon && Img_HeroIcon)
    {
        Img_HeroIcon->SetBrushFromTexture(LoadedIcon);
    }
}
