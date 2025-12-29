#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "System/Data/SFPermanentUpgradeTypes.h"
#include "SFPermanentUpgradeUIDesc.generated.h"

USTRUCT(BlueprintType)
struct SF_API FSFPermanentUpgradeUIDescRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESFUpgradeCategory Category = ESFUpgradeCategory::Wrath;

	// TextBonus용 스탯 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText BonusLabel;

	// 설정한 수치(레벨당 증가량)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BonusPerLevel = 1.0f;

	// Tier 설명 3줄
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Tier1Desc;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Tier2Desc;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Tier3Desc;
};
