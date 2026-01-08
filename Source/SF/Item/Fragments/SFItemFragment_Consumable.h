#pragma once

#include "CoreMinimal.h"
#include "Item/SFItemDefinition.h"
#include "Item/SFItemTypes.h"
#include "SFItemFragment_Consumable.generated.h"

struct FGameplayEffectSpec;
class UGameplayEffect;
/**
 * 
 */
UCLASS()
class SF_API USFItemFragment_Consumable : public USFItemFragment
{
	GENERATED_BODY()

public:
	void ApplySetByCallersToSpec(FGameplayEffectSpec* Spec, const FGameplayTag& RarityTag) const;

public:
	// 사용 시 소모 갯수 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
	int32 ConsumeCount = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> ConsumeEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	TArray<FSFTaggedRarityValues> SetByCallerDatas;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", meta = (Categories = "Montage"))
	FGameplayTag ConsumeMontageTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Type", meta = (Categories = "Consumable"))
	FGameplayTag ConsumeTypeTag;
};
