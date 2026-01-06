// Fill out your copyright notice in the Description page of Project Settings.


#include "SFDropTable.h"

#include "SFItemDefinition.h"
#include "Misc/DataValidation.h"

float FSFDropTableEntry::GetDropChance(float LuckValue) const
{
	float FinalChance = BaseDropChance;

	if (LuckDropChanceCurve)
	{
		float Multiplier = LuckDropChanceCurve->GetFloatValue(LuckValue);
		FinalChance *= Multiplier;
	}

	return FMath::Clamp(FinalChance, 0.f, 1.f);
}

int32 FSFDropTableEntry::RollCount() const
{
	return FMath::RandRange(MinCount, MaxCount);
}

#if WITH_EDITOR
EDataValidationResult USFDropTable::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		const FSFDropTableEntry& Entry = Entries[i];

		if (!Entry.ItemDefinitionClass)
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Entry[%d]: ItemDefinitionClass is null"), i)));
			Result = EDataValidationResult::Invalid;
		}

		if (Entry.MinCount > Entry.MaxCount)
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Entry[%d]: MinCount(%d) > MaxCount(%d)"), i, Entry.MinCount, Entry.MaxCount)));
			Result = EDataValidationResult::Invalid;
		}
	}

	if (GuaranteedDropCount > Entries.Num())
	{
		Context.AddWarning(FText::FromString(FString::Printf(TEXT("GuaranteedDropCount(%d) > Entries.Num()(%d)"), GuaranteedDropCount, Entries.Num())));
	}

	if (MaxDropCount > 0 && MaxDropCount < GuaranteedDropCount)
	{
		Context.AddError(FText::FromString(FString::Printf(TEXT("MaxDropCount(%d) < GuaranteedDropCount(%d)"), MaxDropCount, GuaranteedDropCount)));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif
