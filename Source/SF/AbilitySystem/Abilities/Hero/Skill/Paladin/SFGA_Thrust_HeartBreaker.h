#pragma once

#include "CoreMinimal.h"
#include "SFGA_Thrust_Base.h"
#include "SFGA_Thrust_HeartBreaker.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Thrust_HeartBreaker : public USFGA_Thrust_Base
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnKeyReleased(float TimeHeld);

protected:
	
	// 슈퍼아머 GE
	UPROPERTY(EditDefaultsOnly, Category="Buff")
	TSubclassOf<UGameplayEffect> SuperArmorEffectClass;

	// 최대 차징 시간
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float MaxChargeTime = 3.0f;
};
