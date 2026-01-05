#include "SFItemFragment_StatModifier.h"

#include "Item/SFItemInstance.h"

void USFItemFragment_StatModifier::OnInstanceCreated(USFItemInstance* Instance) const
{
	if (!Instance || !StatData.ValueTag.IsValid())
	{
		return;
	}

	const FGameplayTag& RarityTag = Instance->GetItemRarityTag();
	const float RolledValue = StatData.RollValueForRarity(RarityTag);

	if (RolledValue > 0.f)
	{
		const int32 IntValue = FMath::RoundToInt(RolledValue);
		Instance->AddOrRemoveStatTagStack(StatData.ValueTag, IntValue);
	}
}