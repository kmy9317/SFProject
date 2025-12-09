// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SFGA_Skill_Melee.h"
#include "Interface/SFChainedSkill.h"

#include "SFGA_ChainedSkill_Melee.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_ChainedSkill_Melee : public USFGA_Skill_Melee, public ISFChainedSkill
{
	GENERATED_BODY()
public:
	USFGA_ChainedSkill_Melee(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// ~ Begin ISFChainedSkill
	virtual UAbilitySystemComponent* GetChainASC() const override;
	virtual TSubclassOf<UGameplayEffect> GetComboStateEffectClass() const override { return ComboStateEffectClass; }
	virtual const TArray<FSFChainConfig>& GetChainConfigs() const override { return ChainConfigs; }
	virtual TSubclassOf<UGameplayEffect> GetCooldownEffectClass() const override { return ComboCooldownEffectClass; }
	virtual TArray<FActiveGameplayEffectHandle>& GetAppliedChainEffectHandles() override { return AppliedChainEffectHandles; }
	// ~ End ISFChainedSkill

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void OnTrace(FGameplayEventData Payload) override;
	virtual void ExecuteChainStep(int32 ChainIndex);

	UFUNCTION()
	virtual void OnChainMontageCompleted();

	UFUNCTION()
	virtual void OnChainMontageInterrupted();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "SF|Combo")
	TSubclassOf<UGameplayEffect> ComboStateEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Combo")
	TArray<FSFChainConfig> ChainConfigs;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Combo")
	TSubclassOf<UGameplayEffect> ComboCooldownEffectClass;

	int32 ExecutingChainIndex = 0;
	float CurrentDamageMultiplier = 1.0f;

private:
	TArray<FActiveGameplayEffectHandle> AppliedChainEffectHandles;
};
