#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_Hero_Resurrection.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Hero_Resurrection : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Hero_Resurrection(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// 부활 시 리소스 회복 GE (Health, Stamina, Mana를 Max 기반으로 설정)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Resurrection")
	TSubclassOf<UGameplayEffect> ResurrectionRestoreEffect;

	// 부활 이펙트 GameplayCue 태그 
	UPROPERTY(EditDefaultsOnly, Category = "SF|Resurrection")
	FGameplayTag ResurrectionCueTag;

	// 부활 시 적용할 버프 (무적 등) 
	UPROPERTY(EditDefaultsOnly, Category = "SF|Resurrection")
	TSubclassOf<UGameplayEffect> ResurrectionBuffEffect;
};
