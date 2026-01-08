#include "SFUIData.h"

#include "System/SFAssetManager.h"

const USFUIData& USFUIData::Get()
{
	return USFAssetManager::Get().GetUIData();
}

FText USFUIData::FormatStatValue(float Value, ESFStatDisplayType DisplayType)
{
	switch (DisplayType)
	{
	case ESFStatDisplayType::Integer:
		return FText::AsNumber(FMath::RoundToInt(Value));
        
	case ESFStatDisplayType::Decimal:
		{
			FNumberFormattingOptions Options;
			Options.MinimumFractionalDigits = 1;
			Options.MaximumFractionalDigits = 1;
			return FText::AsNumber(Value, &Options);
		}
        
	case ESFStatDisplayType::Percent:
		return FText::Format(NSLOCTEXT("SF", "PercentFormat", "{0}%"), FMath::RoundToInt(Value * 100));
        
	case ESFStatDisplayType::PerSecond:
		{
			FNumberFormattingOptions Options;
			Options.MinimumFractionalDigits = 1;
			Options.MaximumFractionalDigits = 1;
			return FText::Format(NSLOCTEXT("SF", "PerSecondFormat", "{0}/s"), FText::AsNumber(Value * 10, &Options));
		}
	}
    
	return FText::GetEmpty();
}

const FSFUIInfo& USFUIData::GetTagUIInfoStatic(const FGameplayTag& Tag)
{
	return Get().GetTagUIInfo(Tag);
}

const FSFUIInfo& USFUIData::GetTagUIInfo(const FGameplayTag& Tag) const
{
	if (const FSFUIInfo* Found = TagUIInfos.Find(Tag))
	{
		return *Found;
	}

	static FSFUIInfo EmptyUIInfo;
	return EmptyUIInfo;
}
