#include "SFPlayerStatsEntryWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Player/SFPlayerInfoTypes.h"

void USFPlayerStatsEntryWidget::SetPlayerStats(const FSFPlayerGameStats& Stats)
{
	if (PlayerNameText)
	{
		PlayerNameText->SetText(FText::FromString(Stats.PlayerName));
	}

	if (HeroIconImage && Stats.HeroIcon)
	{
		HeroIconImage->SetBrushFromTexture(Stats.HeroIcon);
	}

	if (DamageText)
	{
		// 천 단위 구분 (예: 12,345)
		FNumberFormattingOptions Options;
		Options.UseGrouping = true;
		DamageText->SetText(FText::AsNumber(FMath::RoundToInt(Stats.TotalDamageDealt), &Options));
	}

	if (DownedCountText)
	{
		DownedCountText->SetText(FText::AsNumber(Stats.DownedCount));
	}

	if (ReviveCountText)
	{
		ReviveCountText->SetText(FText::AsNumber(Stats.ReviveCount));
	}
}