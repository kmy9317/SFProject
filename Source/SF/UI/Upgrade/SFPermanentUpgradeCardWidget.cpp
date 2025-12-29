#include "UI/Upgrade/SFPermanentUpgradeCardWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/Image.h"

void USFPermanentUpgradeCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ButtonPlus)
	{
		ButtonPlus->OnClicked.AddDynamic(this, &USFPermanentUpgradeCardWidget::HandlePlus);
	}
	if (ButtonMinus)
	{
		ButtonMinus->OnClicked.AddDynamic(this, &USFPermanentUpgradeCardWidget::HandleMinus);
	}
}

void USFPermanentUpgradeCardWidget::Setup(
	ESFUpgradeCategory InCategory,
	const FText& InTier1Desc,
	const FText& InTier2Desc,
	const FText& InTier3Desc
)
{
	Category = InCategory;

	if (TextTitle)
	{
		TextTitle->SetText(GetCategoryLabel());
	}

	if (TextTier1) TextTier1->SetText(InTier1Desc);
	if (TextTier2) TextTier2->SetText(InTier2Desc);
	if (TextTier3) TextTier3->SetText(InTier3Desc);
}

void USFPermanentUpgradeCardWidget::Refresh(
	int32 CurrentLevel,
	int32 MaxLevel,
	bool bPlusEnabled,
	bool bMinusEnabled,
	const FText& BonusText
)
{
	if (TextLevel)
	{
		TextLevel->SetText(FText::AsNumber(CurrentLevel));
	}

	if (ButtonPlus)  ButtonPlus->SetIsEnabled(bPlusEnabled);
	if (ButtonMinus) ButtonMinus->SetIsEnabled(bMinusEnabled);

	if (TextBonus)
	{
		TextBonus->SetText(BonusText);
	}

	SetTierVisual(CurrentLevel);
}

void USFPermanentUpgradeCardWidget::HandlePlus()
{
	OnPlusClicked.Broadcast(Category);
}

void USFPermanentUpgradeCardWidget::HandleMinus()
{
	OnMinusClicked.Broadcast(Category);
}

FText USFPermanentUpgradeCardWidget::GetCategoryLabel() const
{
	switch (Category)
	{
	case ESFUpgradeCategory::Wrath: return FText::FromString(TEXT("Wrath"));
	case ESFUpgradeCategory::Pride: return FText::FromString(TEXT("Pride"));
	case ESFUpgradeCategory::Lust:  return FText::FromString(TEXT("Lust"));
	case ESFUpgradeCategory::Sloth: return FText::FromString(TEXT("Sloth"));
	case ESFUpgradeCategory::Greed: return FText::FromString(TEXT("Greed"));
	default: return FText::FromString(TEXT("Unknown"));
	}
}

void USFPermanentUpgradeCardWidget::SetTierVisual(int32 CurrentLevel)
{
	const bool bT1 = (CurrentLevel >= 10);
	const bool bT2 = (CurrentLevel >= 20);
	const bool bT3 = (CurrentLevel >= 30);

	SetTierLine(BorderTier1, ImageTier1Check, bT1);
	SetTierLine(BorderTier2, ImageTier2Check, bT2);
	SetTierLine(BorderTier3, ImageTier3Check, bT3);
}

void USFPermanentUpgradeCardWidget::SetTierLine(UBorder* Border, UImage* CheckImage, bool bActive)
{
	// “활성화 느낌” = 불투명도(밝기) + (선택) 체크 아이콘 표시
	if (Border)
	{
		Border->SetRenderOpacity(bActive ? 1.0f : 0.35f);
	}

	if (CheckImage)
	{
		CheckImage->SetVisibility(bActive ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		CheckImage->SetRenderOpacity(bActive ? 1.0f : 0.0f);
	}
}
