#include "SFAbilityTooltip.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"


void USFAbilityTooltip::SetAbilityInfo(const FName& AbilityName, UTexture2D* AbilityTexture, const FText& AbilityDescription, float AbilityCooldown, float AbilityCost)
{
	AbilityNameText->SetText(FText::FromName(AbilityName));
	AbilityIcon->SetBrushFromTexture(AbilityTexture);

	AbilityDescriptionText->SetText(AbilityDescription);

	FNumberFormattingOptions FormattingOptions;
	FormattingOptions.MaximumFractionalDigits = 0;

	AbilityCooldownText->SetText(FText::AsNumber(AbilityCooldown, &FormattingOptions));
	AbilityCostText->SetText(FText::AsNumber(AbilityCost, &FormattingOptions));
}
