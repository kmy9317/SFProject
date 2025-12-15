#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Skill_Melee.h"

#include "SFGA_Thrust_Salvation.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Thrust_Salvation : public USFGA_Skill_Melee
{
	GENERATED_BODY()

public:
	USFGA_Thrust_Salvation(FObjectInitializer const& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	void ApplyDamageAndKnockback(ASFCharacterBase* Source, ASFCharacterBase* Target, UAbilitySystemComponent* TargetASC);
	void ApplyBuffToAlly(UAbilitySystemComponent* TargetASC);
	
private:
	
	// 돌진 몽타주 완료 후 ShieldBash 몽타주로 전환
	UFUNCTION()
	void OnThrustMontageCompleted();
	
	// 캐릭터 전방에 캡슐 오버랩 검사 수행 및 넉백 효과를 위한 GameplayEvent_Knockback 이벤트 전송
	UFUNCTION()
	void OnShieldBashEffectBegin(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageFinished();

protected:

	UPROPERTY(EditDefaultsOnly, Category="SF|Animation")
	TObjectPtr<UAnimMontage> ThrustMontage;

	UPROPERTY(EditDefaultsOnly, Category="SF|Animation")
	TObjectPtr<UAnimMontage> ShieldBashMontage;

	// 보호막 및 이속 버프 GE
	UPROPERTY(EditDefaultsOnly, Category="SF|Effect")
	TSubclassOf<UGameplayEffect> BuffEffectClass;

	/**
	 * 공격 범위 전방 오프셋 거리
	 * 캐릭터 위치에서 전방으로 이 거리만큼 떨어진 위치에서 오버랩 검사 수행
	 * 공격 중심 = 캐릭터 위치 + (전방 벡터 * Distance)
	 */
	UPROPERTY(EditDefaultsOnly, Category="SF|Effect")
	float Distance = 100.f;

	/**
	 * 공격 범위 반경 배율
	 * 캐릭터 캡슐 반경에 이 값을 곱하여 공격 범위 반경을 결정
	 * 공격 반경 = 캐릭터 캡슐 반경 * RadiusMultiplier
	 */
	UPROPERTY(EditDefaultsOnly, Category="SF|Effect")
	float RadiusMultiplier = 3.25f;
};
