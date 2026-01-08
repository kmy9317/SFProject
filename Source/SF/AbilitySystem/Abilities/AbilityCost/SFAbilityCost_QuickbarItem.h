#pragma once

#include "CoreMinimal.h"
#include "SFAbilityCost.h"
#include "SFAbilityCost_QuickbarItem.generated.h"

UCLASS(meta = (DisplayName = "Quickbar Item"))
class SF_API USFAbilityCost_QuickbarItem : public USFAbilityCost
{
	GENERATED_BODY()

public:
	USFAbilityCost_QuickbarItem();

	virtual bool CheckCost(const USFGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCost(const USFGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
};
