#include "SFHeroOverheadWidget.h"

#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/PanelWidget.h"

void USFHeroOverheadWidget::SetPlayerName(const FString& Name)
{
	// if (NameText)
	// {
	// 	NameText->SetText(FText::FromString(Name));
	// }
}

void USFHeroOverheadWidget::SetReviveGaugeVisible(bool bVisible)
{
	if (ReviveGaugePanel)
	{
		ReviveGaugePanel->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void USFHeroOverheadWidget::SetReviveGaugePercent(float Percent)
{
	if (ReviveProgressBar)
	{
		ReviveProgressBar->SetPercent(FMath::Clamp(Percent, 0.f, 1.f));
	}
}