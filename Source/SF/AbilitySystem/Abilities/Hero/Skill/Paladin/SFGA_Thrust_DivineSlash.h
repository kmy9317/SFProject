#pragma once

#include "CoreMinimal.h"
#include "SFGA_Thrust_Base.h"
#include "SFGA_Thrust_DivineSlash.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Thrust_DivineSlash : public USFGA_Thrust_Base
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 콤보 입력 대기 태스크 활용
	void WaitForNextComboInput();

protected:
	// 무적 효과 GE
	UPROPERTY(EditDefaultsOnly, Category="Buff")
	TSubclassOf<UGameplayEffect> InvincibilityEffectClass;

	// 콤보 카운트
	int32 CurrentComboCount = 0;
};
