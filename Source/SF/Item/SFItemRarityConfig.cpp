#include "SFItemRarityConfig.h"

float USFItemRarityConfig::GetWeightForLuck(float LuckValue) const
{
	float Multiplier = 1.0f;

	if (LuckWeightCurve)
	{
		Multiplier = LuckWeightCurve->GetFloatValue(LuckValue);
	}

	return FMath::Max(0.f, BaseDropWeight * Multiplier);
}
