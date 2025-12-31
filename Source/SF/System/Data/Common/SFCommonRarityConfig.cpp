#include "SFCommonRarityConfig.h"

float USFCommonRarityConfig::GetWeightForLuck(float LuckValue) const
{
	float WeightModifier = 0.0f;

	if (!LuckWeightCurve.IsNull())
	{
		if (UCurveFloat* Curve = LuckWeightCurve.LoadSynchronous())
		{
			WeightModifier = Curve->GetFloatValue(LuckValue);
		}
	}

	return FMath::Max(0.0f, BaseWeight + WeightModifier);
}
