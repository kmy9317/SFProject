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
	virtual float GetTimeoutCooldownDuration() const override;
	virtual float GetCompleteCooldownDuration() const override;
	virtual float GetChainAbilityLevel() const override { return GetAbilityLevel(); }
	// ~ End ISFChainedSkill

	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	
	virtual void OnTrace(FGameplayEventData Payload) override;
	virtual void ExecuteChainStep(int32 ChainIndex);

	UFUNCTION()
	virtual void OnChainMontageCompleted();

	UFUNCTION()
	virtual void OnChainMontageInterrupted();

	void BindComboStateRemovedDelegate();
	void UnbindComboStateRemovedDelegate();
	virtual void OnComboStateRemoved(const FActiveGameplayEffect& RemovedEffect);

protected:
	// 연계 스택 추적용 Effect
	UPROPERTY(EditDefaultsOnly, Category = "SF|Combo")
	TSubclassOf<UGameplayEffect> ComboStateEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Combo")
	TArray<FSFChainConfig> ChainConfigs;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Combo")
	bool bAutoApplyComboState = true;
	
	// ComboState의 OnCompleteNormal에서 사용 (Timeout 쿨다운 Tag==Complete 쿨다운 Tag 필수)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Cooldown")
	TSubclassOf<UGameplayEffect> TimeoutCooldownEffectClass;

	// CompleteCombo에서 사용 (Timeout 쿨다운 Tag==Complete 쿨다운 Tag 필수)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Cooldown")
	TSubclassOf<UGameplayEffect> CompleteCooldownEffectClass;

	// 타임아웃 시 쿨다운 (콤보 중단)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Cooldown", meta = (DisplayName = "타임아웃 쿨타임(초)"))
	FScalableFloat TimeoutCooldownDuration = 3.f;

	// 완료 시 쿨다운 (풀 콤보)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Cooldown", meta = (DisplayName = "완료 쿨타임(초)"))
	FScalableFloat CompleteCooldownDuration = 10.f;
	
	// 쿨다운 체크용 태그 
	UPROPERTY(EditDefaultsOnly, Category = "SF|Cooldown")
	FGameplayTagContainer CooldownTags;

	int32 ExecutingChainIndex = 0;

private:
	TArray<FActiveGameplayEffectHandle> AppliedChainEffectHandles;
	FDelegateHandle ComboStateRemovedHandle;

private:
	// ASC에 쿨타임 GE가 "추가되는 순간" 감지
	FDelegateHandle CooldownGEAddedHandle;

	void OnCooldownGEAdded(
		UAbilitySystemComponent* TargetASC,
		const FGameplayEffectSpec& Spec,
		FActiveGameplayEffectHandle ActiveHandle);

	// Ability 컨텍스트 없이 ASC 기준 쿨초
	void TryProcCooldownReset_FromASC(UAbilitySystemComponent* ASC);

	virtual const FGameplayTagContainer* GetCooldownTags() const override;
};
