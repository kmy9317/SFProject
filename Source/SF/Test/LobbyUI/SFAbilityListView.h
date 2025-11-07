#pragma once

#include "CoreMinimal.h"
#include "Components/ListView.h"
#include "SFAbilityListView.generated.h"

class UGameplayAbility;

USTRUCT(BlueprintType)
struct FSFAbilityWidgetData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AbilityName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;
};

/**
 * 
 */
UCLASS()
class SF_API USFAbilityListView : public UListView
{
	GENERATED_BODY()

public:
	void ConfigureAbilities(TArray<TSubclassOf<UGameplayAbility>> Abilities);

private:
	UPROPERTY(EditAnywhere, Category = "Data")
	UDataTable* AbilityDataTable;

	void AbilityGaugeGenerated(UUserWidget& Widget);

	const FSFAbilityWidgetData* FindWidgetDataForAbility(const TSubclassOf<UGameplayAbility>& AbilityClass) const;
};
