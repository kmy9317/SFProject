#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_Hero_Regen.generated.h"

/**
 * 스태미나/마나 자동 회복 패시브 어빌리티
 * OnSpawn 정책으로 캐릭터 생성 시 자동 활성화
 */
UCLASS()
class SF_API USFGA_Hero_Regen : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Hero_Regen(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,  const FGameplayAbilityActorInfo* ActorInfo,  const FGameplayAbilityActivationInfo ActivationInfo,  const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,  bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 스태미나 리젠 GE
	UPROPERTY(EditDefaultsOnly, Category = "SF|Regen")
	TSubclassOf<UGameplayEffect> StaminaRegenEffectClass;

	// 마나 리젠 GE
	UPROPERTY(EditDefaultsOnly, Category = "SF|Regen")
	TSubclassOf<UGameplayEffect> ManaRegenEffectClass;

private:
	FActiveGameplayEffectHandle StaminaRegenHandle;
	FActiveGameplayEffectHandle ManaRegenHandle;
};
