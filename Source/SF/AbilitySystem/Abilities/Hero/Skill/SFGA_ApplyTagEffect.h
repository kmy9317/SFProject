#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_ApplyTagEffect.generated.h"

/**
 * USFGA_ApplyTagEffect
 * * 부여(GiveAbility) 즉시 발동하여 태그를 정리하고 GE를 적용한 뒤 종료되는 어빌리티
 */
UCLASS()
class SF_API USFGA_ApplyTagEffect : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_ApplyTagEffect();

	// [추가] 어빌리티가 ASC에 등록될 때 호출되는 함수
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
	//~UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	//~End of UGameplayAbility interface

protected:
	/** 부여할 게임플레이 이펙트 클래스입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Effect")
	TSubclassOf<UGameplayEffect> TargetGameplayEffectClass;

	/** 이펙트 부여 시 적용할 레벨입니다. (기본값: 1.0) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Effect")
	float EffectLevel = 1.0f;

	/** * 이 태그가 유효하게 설정되어 있다면, 어빌리티 발동 시 
	 * 해당 태그를 부여하고 있는 모든 액티브 이펙트(Active Gameplay Effect)를 제거합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Effect")
	FGameplayTag TagToRemove;
};