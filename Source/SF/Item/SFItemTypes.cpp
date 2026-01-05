#include "SFItemTypes.h"

float FSFRarityValueRange::RollValue() const
{
	if (FMath::IsNearlyEqual(MinValue, MaxValue))
	{
		return MinValue;
	}
	return FMath::FRandRange(MinValue, MaxValue);
}

bool FSFRarityValueRange::IsFixedValue() const
{
	return FMath::IsNearlyEqual(MinValue, MaxValue);
}

bool FSFTaggedRarityValues::GetRangeForRarity(const FGameplayTag& RarityTag, float& OutMin, float& OutMax) const
{
	for (const FSFRarityValueRange& Range : RarityValues)
	{
		if (Range.RarityTag.MatchesTagExact(RarityTag))
		{
			OutMin = Range.MinValue;
			OutMax = Range.MaxValue;
			return true;
		}
	}
	return false;
}

float FSFTaggedRarityValues::RollValueForRarity(const FGameplayTag& RarityTag) const
{
	for (const FSFRarityValueRange& Range : RarityValues)
	{
		if (Range.RarityTag.MatchesTagExact(RarityTag))
		{
			return Range.RollValue();
		}
	}
	return 0.f;
}

float FSFTaggedRarityValues::GetFixedValueForRarity(const FGameplayTag& RarityTag) const
{
	for (const FSFRarityValueRange& Range : RarityValues)
	{
		if (Range.RarityTag.MatchesTagExact(RarityTag))
		{
			return Range.MinValue;
		}
	}
	return 0.f;
}
