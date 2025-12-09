#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SFChainedSkill.generated.h"

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

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Cost")
	TSubclassOf<UGameplayEffect> CostEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TArray<TSubclassOf<UGameplayEffect>> ChainEffects;
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
	virtual TSubclassOf<UGameplayEffect> GetCooldownEffectClass() const = 0;
	virtual TArray<FActiveGameplayEffectHandle>& GetAppliedChainEffectHandles() = 0;

	int32 GetCurrentChain() const;

	bool CanContinueChain() const;

	bool IsLastChain(int32 StepIndex) const;

	FActiveGameplayEffectHandle ApplyComboState(UGameplayAbility* SourceAbility);
	void RemoveComboState();

	bool ApplyChainCost(int32 ChainIndex, UGameplayAbility* SourceAbility);
	void ApplyChainEffects(int32 ChainIndex, UGameplayAbility* SourceAbility);

	void RemoveChainEffects();
	void CompleteCombo(UGameplayAbility* SourceAbility);
};
