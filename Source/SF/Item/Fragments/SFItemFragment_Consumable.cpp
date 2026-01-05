#include "SFItemFragment_Consumable.h"

#include "GameplayEffect.h"

void USFItemFragment_Consumable::ApplySetByCallersToSpec(FGameplayEffectSpec* Spec, const FGameplayTag& RarityTag) const
{
	if (!Spec)
	{
		return;
	}

	for (const FSFTaggedRarityValues& Data : SetByCallerDatas)
	{
		if (Data.ValueTag.IsValid())
		{
			const float Magnitude = Data.GetFixedValueForRarity(RarityTag);
			Spec->SetSetByCallerMagnitude(Data.ValueTag, Magnitude);
		}
	}
}