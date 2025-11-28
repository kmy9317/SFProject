#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Skill_Melee.h"
#include "SFGA_Thrust_Base.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Thrust_Base : public USFGA_Skill_Melee
{
	GENERATED_BODY()

public:
	USFGA_Thrust_Base(FObjectInitializer const& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void OnTrace(FGameplayEventData Payload);
	
	UFUNCTION()
	void OnThrustBegin(FGameplayEventData Payload);
	
	UFUNCTION()
	void OnThrustEnd(FGameplayEventData Payload);
	
	UFUNCTION()
	void OnMontageFinished();
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "SF|Animation")
	TObjectPtr<UAnimMontage> ThrustMontage;

	UPROPERTY(EditDefaultsOnly, Category="SF|Damage")
	float BaseDamage = 10.f;
};
