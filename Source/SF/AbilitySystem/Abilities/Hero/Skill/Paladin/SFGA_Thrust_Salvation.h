#pragma once

#include "CoreMinimal.h"
#include "SFGA_Thrust_Base.h"
#include "SFGA_Thrust_Salvation.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Thrust_Salvation : public USFGA_Thrust_Base
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 가장 체력이 낮은 아군 찾기
	AActor* FindLowestHPAlly();

protected:
	
	// 보호막 및 이속 버프 GE
	UPROPERTY(EditDefaultsOnly, Category="Buff")
	TSubclassOf<UGameplayEffect> BuffEffectClass;
};
