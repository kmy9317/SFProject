#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/SFGA_Equipment_Base.h"
#include "ScalableFloat.h" 
#include "SFGA_AreaOverTimeSkill.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;

// 빙결 데이터 관리 구조체
struct FFrozenTargetData
{
	FTimerHandle UnfreezeTimerHandle;
	float OriginalTimeDilation;
	
	FFrozenTargetData() : OriginalTimeDilation(1.0f) {}
};

/**
 * 범위 지속 데미지 및 빙결 스킬
 * 사용자 요청: ASFGroundAOE의 타격 로직을 그대로 이식
 */
UCLASS()
class SF_API USFGA_AreaOverTimeSkill : public USFGA_Equipment_Base
{
	GENERATED_BODY()

public:
	USFGA_AreaOverTimeSkill();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	void PlaySkillAnimation();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnGameplayEventReceived(FGameplayEventData Payload);

	void StartAttackLoop();
	void PerformAreaTick();
	void OnDurationExpired();

	/** * [핵심] GroundAOE의 ApplyDamageToTargets 로직을 이식한 함수 
	 */
	void ApplyDamageToTargets(float DamageAmount, float EffectRadius);

	// 빙결 관련 함수
	void FreezeTarget(AActor* TargetActor);
	void UnfreezeTarget(TWeakObjectPtr<AActor> TargetWeakPtr);
	void ClearAllFrozenTargets();

protected:
	// --- 설정 변수 ---
	
	UPROPERTY(EditDefaultsOnly, Category = "SF|Skill|Animation")
	TObjectPtr<UAnimMontage> SkillMontage;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Skill|Animation")
	FGameplayTag TriggerEventTag;

	// 레벨별 변동 데미지
	UPROPERTY(EditDefaultsOnly, Category = "SF|Skill|Combat")
	FScalableFloat BaseDamage;

	// 데미지 전달용 태그
	UPROPERTY(EditDefaultsOnly, Category = "SF|Skill|Combat")
	FGameplayTag DamageSetByCallerTag;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Skill|Combat")
	float AttackDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Skill|Combat")
	float DamageInterval = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Skill|Combat")
	float Radius = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Skill|Combat")
	TSubclassOf<UGameplayEffect> DamageGameplayEffectClass; // 변수명 GroundAOE와 통일

	UPROPERTY(EditDefaultsOnly, Category = "SF|Skill|Combat")
	TSubclassOf<UGameplayEffect> DebuffGameplayEffectClass; // 변수명 GroundAOE와 통일

	UPROPERTY(EditDefaultsOnly, Category = "SF|Skill|Freeze")
	bool bEnableFreeze = false;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Skill|VFX")
	FGameplayTag LoopingGameplayCueTag;

private:
	FTimerHandle LoopTimerHandle;
	FTimerHandle DurationTimerHandle;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitEventTask;

	TMap<TWeakObjectPtr<AActor>, FFrozenTargetData> FrozenTargetMap;
};