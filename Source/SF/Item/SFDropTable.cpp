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

int32 FSFDropTableEntry::RollSpawnCount() const
{
	return FMath::RandRange(MinSpawnCount, MaxSpawnCount);
}

int32 FSFDropTableEntry::RollAmountPerSpawn() const
{
	return FMath::RandRange(MinAmountPerSpawn, MaxAmountPerSpawn);
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

		// MinCount/MaxCount → MinSpawnCount/MaxSpawnCount
		if (Entry.MinSpawnCount > Entry.MaxSpawnCount)
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Entry[%d]: MinSpawnCount(%d) > MaxSpawnCount(%d)"), i, Entry.MinSpawnCount, Entry.MaxSpawnCount)));
			Result = EDataValidationResult::Invalid;
		}

		// AmountPerSpawn 검증 추가
		if (Entry.MinAmountPerSpawn > Entry.MaxAmountPerSpawn)
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Entry[%d]: MinAmountPerSpawn(%d) > MaxAmountPerSpawn(%d)"), i, Entry.MinAmountPerSpawn, Entry.MaxAmountPerSpawn)));
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
