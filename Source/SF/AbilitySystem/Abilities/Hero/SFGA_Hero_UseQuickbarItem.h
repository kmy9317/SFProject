#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_Hero_UseQuickbarItem.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Hero_UseQuickbarItem : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Hero_UseQuickbarItem(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
