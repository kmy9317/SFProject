#include "SFAbilityListView.h"

#include "Abilities/GameplayAbility.h"

void USFAbilityListView::ConfigureAbilities(TArray<TSubclassOf<UGameplayAbility>> Abilities)
{
	for (TSubclassOf<UGameplayAbility>& Ability : Abilities)
	{
		AddItem(Ability);
	}
}

void USFAbilityListView::AbilityGaugeGenerated(UUserWidget& Widget)
{
	
}

const FSFAbilityWidgetData* USFAbilityListView::FindWidgetDataForAbility(const TSubclassOf<UGameplayAbility>& AbilityClass) const
{
	if (!AbilityDataTable)
		return nullptr;

	for (auto& AbilityWidgetDataPair : AbilityDataTable->GetRowMap())
	{
		const FSFAbilityWidgetData* WidgetData = AbilityDataTable->FindRow<FSFAbilityWidgetData>(AbilityWidgetDataPair.Key, "");
		if (WidgetData->AbilityClass == AbilityClass)
		{
			return WidgetData;
		}
	}

	return nullptr;
}