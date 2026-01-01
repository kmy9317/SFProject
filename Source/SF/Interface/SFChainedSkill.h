#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Character/Hero/Component/SFHeroMovementComponent.h"
#include "UObject/Interface.h"
#include "SFChainedSkill.generated.h"

struct FActiveGameplayEffect;
class UGameplayAbility;
struct FActiveGameplayEffectHandle;
class UAbilitySystemComponent;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct SF_API FSFChainConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Cost")
	TSubclassOf<UGameplayEffect> CostEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TArray<TSubclassOf<UGameplayEffect>> ChainEffects;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	ESFSlidingMode ChainSlidingMode;
};


// This class does not need to be modified.
UINTERFACE()
class USFChainedSkill : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SF_API ISFChainedSkill
{
	GENERATED_BODY()

public:
	virtual UAbilitySystemComponent* GetChainASC() const = 0;
	virtual TSubclassOf<UGameplayEffect> GetComboStateEffectClass() const = 0;
	virtual const TArray<FSFChainConfig>& GetChainConfigs() const = 0;
	virtual TSubclassOf<UGameplayEffect> GetTimeoutCooldownEffectClass() const = 0;
	virtual TSubclassOf<UGameplayEffect> GetCompleteCooldownEffectClass() const = 0;
	virtual TArray<FActiveGameplayEffectHandle>& GetAppliedChainEffectHandles() = 0;
	virtual FGameplayTagContainer GetChainedSkillCooldownTags() const = 0;

	virtual float GetTimeoutCooldownDuration() const = 0;
	virtual float GetCompleteCooldownDuration() const = 0;
	virtual float GetChainAbilityLevel() const = 0;

	virtual UTexture2D* GetChainIcon(int32 ChainIndex) const;
	virtual void CompleteCombo(UGameplayAbility* SourceAbility);
	virtual void ApplyTimeoutCooldown(UGameplayAbility* SourceAbility);
	
	int32 GetCurrentChain() const;
	bool CanContinueChain() const;
	bool IsLastChain(int32 StepIndex) const;

	FActiveGameplayEffectHandle ApplyComboState(UGameplayAbility* SourceAbility, int32 NextChainIndex);
	void RemoveComboState(UGameplayAbility* SourceAbility);

	// ComboState 제거 시 호출 (구현 클래스에서 델리게이트 콜백으로 호출)
	bool HandleComboStateRemoved(UGameplayAbility* SourceAbility, const FActiveGameplayEffect& RemovedEffect);

	bool ApplyChainCost(int32 ChainIndex, UGameplayAbility* SourceAbility);
	void ApplyChainEffects(int32 ChainIndex, UGameplayAbility* SourceAbility);
	void RemoveChainEffects();
	
	// 쿨다운 적용 공통 헬퍼 (SetByCaller 방식)
	void ApplyChainCooldownInternal(UGameplayAbility* SourceAbility, TSubclassOf<UGameplayEffect> CooldownGEClass, float Duration);

protected:
	void BroadcastChainStateChanged(UGameplayAbility* SourceAbility, int32 ChainIndex);

};
