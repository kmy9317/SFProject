// Fill out your copyright notice in the Description page of Project Settings.


#include "SFPawnData.h"

USFPawnData::USFPawnData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PawnClass = nullptr;
	InputConfig = nullptr;
	DefaultCameraMode = nullptr;
}

TArray<TSubclassOf<USFGameplayAbility>> USFPawnData::GetUpgradeOptionsForSlot(FGameplayTag InputTag) const
{
	if (const FSFSkillUpgradeOptionList* OptionList = SkillUpgradeMap.Find(InputTag))
	{
		return OptionList->UpgradeAbilities;
	}
	return TArray<TSubclassOf<USFGameplayAbility>>();
}

FGameplayTag USFPawnData::GetUpgradeSlotTagForStage(int32 StageIndex) const
{
	if (UpgradeSlotOrder.IsValidIndex(StageIndex))
	{
		return UpgradeSlotOrder[StageIndex];
	}
	return FGameplayTag();
}
