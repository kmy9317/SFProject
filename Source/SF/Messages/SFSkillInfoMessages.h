#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "SFSkillInfoMessages.generated.h"

class UTextBlock;
class UProgressBar;

USTRUCT(BlueprintType)
struct FSFSkillProgressInfoMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	bool bShouldShow = false;

	UPROPERTY(BlueprintReadWrite)
	FText DisplayName = FText::GetEmpty();

	UPROPERTY(BlueprintReadWrite)
	FLinearColor PhaseColor = FLinearColor::White;

	UPROPERTY(BlueprintReadWrite)
	float TotalSkillTime = 0.f;
};

USTRUCT(BlueprintType)
struct FSFSkillProgressRefreshMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FLinearColor PhaseColor = FLinearColor::White;
};

USTRUCT(BlueprintType)
struct FSFChainStateChangedMessage
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayAbilitySpecHandle AbilitySpecHandle;

	UPROPERTY()
	int32 ChainIndex = 0;
};
