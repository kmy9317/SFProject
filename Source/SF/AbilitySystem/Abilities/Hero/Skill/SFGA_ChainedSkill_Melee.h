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
	virtual TSubclassOf<UGameplayEffect> GetTimeoutCooldownEffectClass() const override { return TimeoutCooldownEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetCompleteCooldownEffectClass() const override { return CompleteCooldownEffectClass; }
	virtual TArray<FActiveGameplayEffectHandle>& GetAppliedChainEffectHandles() override { return AppliedChainEffectHandles; }
	virtual FGameplayTagContainer GetChainedSkillCooldownTags() const override { return CooldownTags; }
	// ~ End ISFChainedSkill

	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;
	
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
	// 연계 스택 추적용 Effect
	UPROPERTY(EditDefaultsOnly, Category = "SF|Combo")
	TSubclassOf<UGameplayEffect> ComboStateEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Combo")
	TArray<FSFChainConfig> ChainConfigs;

	// ComboState의 OnCompleteNormal에서 사용 (Timeout 쿨다운 Tag==Complete 쿨다운 Tag 필수)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Cooldown")
	TSubclassOf<UGameplayEffect> TimeoutCooldownEffectClass;

	// CompleteCombo에서 사용 (Timeout 쿨다운 Tag==Complete 쿨다운 Tag 필수)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Cooldown")
	TSubclassOf<UGameplayEffect> CompleteCooldownEffectClass;

	// 쿨다운 체크용 태그 
	UPROPERTY(EditDefaultsOnly, Category = "SF|Cooldown")
	FGameplayTagContainer CooldownTags;

	int32 ExecutingChainIndex = 0;
	float CurrentDamageMultiplier = 1.0f;

private:
	TArray<FActiveGameplayEffectHandle> AppliedChainEffectHandles;
};
