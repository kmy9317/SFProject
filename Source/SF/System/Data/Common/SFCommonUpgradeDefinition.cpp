#include "SFCommonUpgradeDefinition.h"

bool USFCommonUpgradeDefinition::IsAllowedForRarity(const FGameplayTag& RarityTag) const
{
	// 비어있으면 모든 등급 허용
	if (AllowedRarityTags.IsEmpty())
	{
		return true;
	}
	return AllowedRarityTags.HasTagExact(RarityTag);
}
