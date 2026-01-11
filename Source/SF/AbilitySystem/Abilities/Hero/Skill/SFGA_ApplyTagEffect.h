#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_ApplyTagEffect.generated.h"

UCLASS()
class SF_API USFGA_ApplyTagEffect : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_ApplyTagEffect();
	
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Effect")
	TSubclassOf<UGameplayEffect> TargetGameplayEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Effect")
	float EffectLevel = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Effect")
	FGameplayTag TagToAdd;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Effect")
	FGameplayTag TagToRemove;
};