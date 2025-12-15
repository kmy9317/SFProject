#include "SFPlayerInfoWidget.h"

#include "IMediaTracks.h"
#include "Components/TextBlock.h"
#include "Components/RetainerBox.h"

void USFPlayerInfoWidget::UpdatePlayerInfo(const FSFPlayerInfo& NewPlayerInfo)
{
	// PlayerInfo 저장
	PlayerInfo = NewPlayerInfo;

	// === Player Name 설정 ===
	if (Text_PlayerName)
	{
		Text_PlayerName->SetText(FText::FromString(PlayerInfo.PlayerName));
	}

	// === Ready Status 설정 ===
	if (Text_ReadyStatus)
	{
		if (PlayerInfo.bReady)
		{
			Text_ReadyStatus->SetText(FText::FromString(TEXT("READY")));
			Text_ReadyStatus->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
		}
		else
		{
			Text_ReadyStatus->SetText(FText::FromString(TEXT("WAIT")));
			Text_ReadyStatus->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		}
	}

	// === Opacity 설정 ===
	if (RetainerBox)
	{
		float Opacity = PlayerInfo.bReady ? 1.0f : 0.7f;
		RetainerBox->SetRenderOpacity(Opacity);
	}
}

