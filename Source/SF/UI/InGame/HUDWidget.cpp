#include "UI/InGame/HUDWidget.h"
#include "UI/Common/CommonBarBase.h"

void UHUDWidget::UpdateHp(float InPercent)
{
	if (HpBar)
	{
		HpBar->SetPercentVisuals(InPercent);
	}
}

void UHUDWidget::UpdateMp(float InPercent)
{
	if (MpBar)
	{
		MpBar->SetPercentVisuals(InPercent);
	}
}

void UHUDWidget::UpdateSp(float InPercent)
{
	if (SpBar)
	{
		SpBar->SetPercentVisuals(InPercent);
	}
}

