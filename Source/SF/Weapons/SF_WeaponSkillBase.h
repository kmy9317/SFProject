#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SF_WeaponSkillBase.generated.h"


UCLASS(Abstract) 
class SF_API UWeaponSkillBase : public UGameplayAbility 
{
	GENERATED_BODY()

public:
	UWeaponSkillBase();

	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> MontageToPlay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SF|Combat")
	float WeaponBaseDamage = 10.0f;

protected:
	
	 
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	
	 
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	virtual void OnMontageCompleted();

	
	UFUNCTION()
	virtual void OnMontageInterrupted();
	
	UFUNCTION()
	virtual void OnMontageCancelled();

protected:
	
	UPROPERTY()
	TObjectPtr<class UAbilityTask_PlayMontageAndWait> MontageTask;
};