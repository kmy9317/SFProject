#include "SFHeroEntryWidget.h"

#include "Character/Hero/SFHeroDefinition.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void USFHeroEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	HeroDefinition = Cast<USFHeroDefinition>(ListItemObject);
	if (HeroDefinition)
	{
		HeroIcon->GetDynamicMaterial()->SetTextureParameterValue(IconTextureMatParamName, HeroDefinition->LoadIcon());
		HeroNameText->SetText(FText::FromString(HeroDefinition->GetHeroDisplayName()));
	}
}

void USFHeroEntryWidget::SetSelected(bool bIsSelected)
{
	HeroIcon->GetDynamicMaterial()->SetScalarParameterValue(SaturationMatParamName, bIsSelected ? 0.f : 1.f);
}
