#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFGA_ChainedSkill_Melee.h"
#include "SFGA_Thrust_DivineSlash.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Thrust_DivineSlash : public USFGA_ChainedSkill_Melee
{
	GENERATED_BODY()

public:
	USFGA_Thrust_DivineSlash(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ExecuteChainStep(int32 ChainIndex) override;
	virtual void OnChainMontageCompleted() override;
	virtual void OnChainMontageInterrupted() override;
	
	UFUNCTION()
	void OnInvincibilityStart(FGameplayEventData Payload);

	UFUNCTION()
	void OnInvincibilityEnd(FGameplayEventData Payload);

	void ApplyInvincibility();
	void RemoveInvincibility();
protected:
	// 무적 효과 GE
	UPROPERTY(EditDefaultsOnly, Category="SF|Effect")
	TSubclassOf<UGameplayEffect> InvincibilityEffectClass;

private:
	FActiveGameplayEffectHandle InvincibilityEffectHandle;
};
