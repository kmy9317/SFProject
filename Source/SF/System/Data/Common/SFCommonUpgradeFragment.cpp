#include "SFCommonUpgradeFragment.h"

float USFCommonUpgradeFragment_StatBoost::GetRandomMagnitudeForRarity(const FGameplayTag& RarityTag) const
{
	for (const FSFRarityMagnitudeRange& Range : RarityMagnitudeRanges)
	{
		if (Range.RarityTag.MatchesTagExact(RarityTag))
		{
			return GetSteppedRandomValue(Range.MinMagnitude, Range.MaxMagnitude);
		}
	}

	if (RarityMagnitudeRanges.Num() > 0)
	{
		return GetSteppedRandomValue(RarityMagnitudeRanges[0].MinMagnitude, RarityMagnitudeRanges[0].MaxMagnitude);
	}

	return 0.0f;
}

float USFCommonUpgradeFragment_StatBoost::GetSteppedRandomValue(float Min, float Max) const
{
	// DecimalPlaces=0 → Step=1, DecimalPlaces=2 → Step=0.01
	float Step = FMath::Pow(10.0f, -DecimalPlaces);
	int32 MinSteps = FMath::RoundToInt(Min / Step);
	int32 MaxSteps = FMath::RoundToInt(Max / Step);
	int32 RandomSteps = FMath::RandRange(MinSteps, MaxSteps);
	return RandomSteps * Step;
}

float USFCommonUpgradeFragment_StatBoost::GetDisplayValue(float RawValue) const
{
	switch (DisplayType)
	{
	case ESFUpgradeDisplayType::Percent:
		return RawValue * 100.0f;
	case ESFUpgradeDisplayType::PerSecond:
		return (RegenTickInterval > 0.0f) ? (RawValue / RegenTickInterval) : RawValue;
	default:
		return RawValue;
	}
}

FText USFCommonUpgradeFragment_StatBoost::FormatDisplayValue(float RawValue) const
{
	float DisplayValue = GetDisplayValue(RawValue);

	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 0;
	Options.MaximumFractionalDigits = DecimalPlaces;

	FText ValueText = FText::AsNumber(DisplayValue, &Options);

	switch (DisplayType)
	{
	case ESFUpgradeDisplayType::Percent:
		return FText::Format(NSLOCTEXT("SF", "PercentFormat", "+{0}%"), ValueText);
	case ESFUpgradeDisplayType::PerSecond:
		return FText::Format(NSLOCTEXT("SF", "PerSecondFormat", "+{0}/초"), ValueText);
	default:
		return FText::Format(NSLOCTEXT("SF", "RawFormat", "+{0}"), ValueText);
	}
}
