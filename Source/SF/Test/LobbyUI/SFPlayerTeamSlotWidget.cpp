#include "SFPlayerTeamSlotWidget.h"

#include "Character/Hero/SFHeroDefinition.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void USFPlayerTeamSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	PlayerHeroIcon->GetDynamicMaterial()->SetScalarParameterValue(HeroEmptyMatParamName, 1);
	CachedHeroNameStr = "";
}

void USFPlayerTeamSlotWidget::UpdateSlot(const FString& PlayerName, const USFHeroDefinition* HeroDefinition)
{
	CachedPlayerNameStr = PlayerName;

	if (HeroDefinition)
	{
		PlayerHeroIcon->GetDynamicMaterial()->SetTextureParameterValue(HeroIconMatParamName, HeroDefinition->LoadIcon());
		PlayerHeroIcon->GetDynamicMaterial()->SetScalarParameterValue(HeroEmptyMatParamName, 0);
		CachedHeroNameStr = HeroDefinition->GetHeroDisplayName();
	}
	else
	{
		PlayerHeroIcon->GetDynamicMaterial()->SetScalarParameterValue(HeroEmptyMatParamName, 1);
		CachedHeroNameStr = "";
	}

	UpdateNameText();
}

void USFPlayerTeamSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	NameText->SetText(FText::FromString(CachedHeroNameStr));
	PlayAnimationForward(HoverAnim);
}

void USFPlayerTeamSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	NameText->SetText(FText::FromString(CachedPlayerNameStr));
	PlayAnimationReverse(HoverAnim);
}

void USFPlayerTeamSlotWidget::UpdateNameText()
{
	if (IsHovered())
	{
		NameText->SetText(FText::FromString(CachedHeroNameStr));
	}
	else
	{
		NameText->SetText(FText::FromString(CachedPlayerNameStr));
	}
}