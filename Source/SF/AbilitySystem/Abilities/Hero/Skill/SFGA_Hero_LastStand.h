#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_Hero_LastStand.generated.h"

UCLASS()
class SF_API USFGA_Hero_LastStand : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Hero_LastStand();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> HealEffect;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> InvincibleEffect;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> DamageBoostEffect;
};