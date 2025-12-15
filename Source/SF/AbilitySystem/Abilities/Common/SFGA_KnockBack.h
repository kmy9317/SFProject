#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_KnockBack.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_KnockBack : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_KnockBack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 공격자와 타겟의 상대적 각도를 계산하여 적절한 방향의 넉백 몽타주 선택
	UAnimMontage* SelectDirectionalMontage(const AActor* Source, const AActor* Target) const;

private:
	UFUNCTION()
	void OnKnockbackFinished();

protected:

	UPROPERTY(EditDefaultsOnly, Category="SF|Knockback")
	float KnockbackStrength = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category="SF|Knockback")
	float KnockbackDuration = 0.3f;

	// 시간에 따른 넉백 강도 변화를 정의하는 커브(nullptr이면 일정한 강도 적용)
	UPROPERTY(EditDefaultsOnly, Category="SF|Knockback")
	TObjectPtr<UCurveFloat> KnockbackStrengthCurve;

	// 앞으로 밀리기 위한 판정 임계값(도) (|DeltaYaw| < ForwardThreshold 이면 앞으로 밀림)
	UPROPERTY(EditDefaultsOnly, Category="SF|Knockback")
	float ForwardThreshold = 50.f;

	// 뒤로 밀리기 위한 판정 임계값(도) (|DeltaYaw| > BackwardThreshold 이면 뒤로 밀림)
	UPROPERTY(EditDefaultsOnly, Category="SF|Knockback")
	float BackwardThreshold = 130.f;

protected:
	
	//------------------------------------------------------------------
	// 방향별 넉백 애니메이션 몽타주
	//------------------------------------------------------------------

	// 앞으로 밀릴 때
	UPROPERTY(EditDefaultsOnly, Category="SF|Animation")
	TObjectPtr<UAnimMontage> KnockbackForwardMontage;
	
	// 뒤로 밀릴 때
	UPROPERTY(EditDefaultsOnly, Category="SF|Animation")
	TObjectPtr<UAnimMontage> KnockbackBackwardMontage;

	// 왼쪽으로 밀릴 때
	UPROPERTY(EditDefaultsOnly, Category="SF|Animation")
	TObjectPtr<UAnimMontage> KnockbackLeftMontage;

	// 오른쪽으로 밀릴 때
	UPROPERTY(EditDefaultsOnly, Category="SF|Animation")
	TObjectPtr<UAnimMontage> KnockbackRightMontage;
};
