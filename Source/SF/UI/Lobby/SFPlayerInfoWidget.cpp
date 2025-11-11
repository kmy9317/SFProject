#include "SFPlayerInfoWidget.h"
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
		FString ReadyText = PlayerInfo.bReady ? TEXT("Ready") : TEXT("Not Ready");
		Text_ReadyStatus->SetText(FText::FromString(ReadyText));
	}

	// === Opacity 설정 ===
	if (RetainerBox)
	{
		float Opacity = PlayerInfo.bReady ? 1.0f : 0.2f;
		RetainerBox->SetRenderOpacity(Opacity);
	}
}

