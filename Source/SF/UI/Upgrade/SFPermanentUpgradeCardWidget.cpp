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
	case ESFUpgradeCategory::Wrath: return FText::FromString(TEXT("분노"));
	case ESFUpgradeCategory::Pride: return FText::FromString(TEXT("오만"));
	case ESFUpgradeCategory::Lust:  return FText::FromString(TEXT("색욕"));
	case ESFUpgradeCategory::Sloth: return FText::FromString(TEXT("나태"));
	case ESFUpgradeCategory::Greed: return FText::FromString(TEXT("탐욕"));
	default: return FText::FromString(TEXT("미상"));
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
		const FLinearColor ActiveColor(1.0f, 0.75, 0.0f, 1.0f);
		const FLinearColor InactiveColor = FLinearColor::White;
		
		Border->SetBrushColor(bActive ? ActiveColor : InactiveColor);
	}

	if (CheckImage)
	{
		CheckImage->SetVisibility(bActive ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		CheckImage->SetRenderOpacity(bActive ? 1.0f : 0.0f);
	}
}
