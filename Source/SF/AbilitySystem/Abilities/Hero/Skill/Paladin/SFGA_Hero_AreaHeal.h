#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Hero_Skill_Buff.h"
#include "GameplayTagContainer.h"
#include "SFGA_Hero_AreaHeal.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class ASFCharacterBase;
struct FTimerHandle;
struct FActiveGameplayEffectHandle;

UCLASS()
class SF_API USFGA_Hero_AreaHeal : public USFGA_Hero_Skill_Buff
{
	GENERATED_BODY()

public:
	USFGA_Hero_AreaHeal(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	//스킬 적용 범위
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|AreaHeal")
	float HealRadius = 600.f;

	//시전자 효과 적용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|AreaHeal")
	bool bIncludeSelf = true;

	//스킬 지속 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|AreaHeal")
	float AreaDuration = 7.f;

	//적용 여부 갱신 주기(범위 안에 있는지)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|AreaHeal")
	float CheckInterval = 0.5f;

	//힐 버프에 영향받을 때 재생되는 오라 FX (개별 캐릭터용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|AreaHeal|Visual")
	UNiagaraSystem* TargetHealNiagaraSystem = nullptr;

	//애님 노티파이에서 쏠 GameplayEvent 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|AreaHeal|GameplayEvent")
	FGameplayTag HealEventTag;

	//태그 이벤트 수신 → 스킬, 이펙트, 사운드 시작
	UFUNCTION()
	void OnHealEventReceived(FGameplayEventData Payload);

	//실제 힐 장판/버프 적용 로직 시작
	void StartAreaHeal(const FGameplayAbilityActorInfo* ActorInfo);

	//부모 쪽 이펙트 & 사운드 자동 재생 막기용
	virtual void SpawnBuffVisualsAndAudio(const FGameplayAbilityActorInfo* ActorInfo) override;

	//================내부 상태=================
	UPROPERTY()
	FVector FixedOrigin;

	FTimerHandle AreaTickTimerHandle;
	FTimerHandle AreaDurationTimerHandle;

	UPROPERTY()
	TMap<ASFCharacterBase*, FActiveGameplayEffectHandle> ActiveHealEffects;

	UPROPERTY()
	TMap<ASFCharacterBase*, UNiagaraComponent*> ActiveHealAuras;
	//==========================================

protected:
	//범위 안/밖 캐릭터 판정 + GE/FX 적용/제거
	void UpdateAreaHeal();

	//스킬(장판) 지속 시간이 끝났을 때
	void OnAreaDurationFinished();

	//버프용 기본 훅(AreaHeal은 사용 안 함)
	virtual void ApplyBuffEffectToTargets(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

public:
	//Ability 실행
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	//Ability 종료
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;
};
