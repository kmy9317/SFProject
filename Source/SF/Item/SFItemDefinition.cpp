#include "SFItemDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

USFItemDefinition::USFItemDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
    
}

#if WITH_EDITOR
EDataValidationResult USFItemDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = UObject::IsDataValid(Context);
	if (MaxStackCount < 1)
	{
		Context.AddError(FText::FromString(FString::Printf(TEXT("MaxStackCount is less than 1"))));
		Result = EDataValidationResult::Invalid;
	}

	if (!Icon)
	{
		Context.AddWarning(FText::FromString(TEXT("Icon is not set")));
	}

	for (int32 i = 0; i < Fragments.Num(); ++i)
	{
		if (!Fragments[i])
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Fragment[%d] is null"), i)));
			Result = EDataValidationResult::Invalid;
		}
	}
	
	return Result;
}
#endif // WITH_EDITOR

const USFItemFragment* USFItemDefinition::FindFragmentByClass(TSubclassOf<USFItemFragment> FragmentClass) const
{
	if (FragmentClass)
	{
		for (USFItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				return Fragment;
			}
		}
	}
	return nullptr;
}

bool USFItemDefinition::CanDropWithRarity(const FGameplayTag& RarityTag) const
{
	// 제한 없으면 모든 등급 허용
	if (AllowedRarities.IsEmpty())
	{
		return true;
	}

	return AllowedRarities.HasTagExact(RarityTag);
}
