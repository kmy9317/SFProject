#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SFGameData.generated.h"

class USFCommonLootTable;
class UGameplayEffect;

/**
 * 
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "SF Game Data", ShortTooltip = "Data asset containing global game data."))
class SF_API USFGameData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	USFGameData();
	
public:
	static const USFGameData& Get();

public:
	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects", meta = (DisplayName = "Damage Gameplay Effect (SetByCaller)"))
	TSoftClassPtr<UGameplayEffect> DamageGameplayEffect_SetByCaller;

	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects", meta = (DisplayName = "IncomingDamage Gameplay Effect (SetByCaller)"))
	TSoftClassPtr<UGameplayEffect> IncomingDamageGameplayEffect_SetByCaller;
	
	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects", meta = (DisplayName = "Heal Gameplay Effect (SetByCaller)"))
	TSoftClassPtr<UGameplayEffect> HealGameplayEffect_SetByCaller;
	
	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects")
	TSoftClassPtr<UGameplayEffect> DynamicTagGameplayEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects")
	TSoftClassPtr<UGameplayEffect> AttributeModifierGameplayEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Common Upgrade")
	TSoftObjectPtr<USFCommonLootTable> DefaultCommonLootTable;

};
