#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Hero_Skill_Buff.h"
#include "SFGA_Hero_AreaHeal_A.generated.h"

class UNiagaraSystem;

UCLASS()
class USFGA_Hero_AreaHeal_A : public USFGA_Hero_Skill_Buff
{
	GENERATED_BODY()

public:
	USFGA_Hero_AreaHeal_A(const FObjectInitializer& ObjectInitializer);

protected:
	//스킬 영역 고정 좌표(시전 위치)
	FVector FixedOrigin;

	//효과 적용 시전자 포함 여부
	UPROPERTY(EditDefaultsOnly, Category="SF|Area")
	bool bIncludeSelf = true;

	//스킬 반경
	UPROPERTY(EditDefaultsOnly, Category="SF|Area")
	float AreaRadius = 400.f;

	//스킬 유지 시간
	UPROPERTY(EditDefaultsOnly, Category="SF|Area")
	float AreaDuration = 8.f;

	//영역 진입 여부 체크 간격
	UPROPERTY(EditDefaultsOnly, Category="SF|Area")
	float CheckInterval = 0.5f;

	//FX (범위 안 대상 오라)
	UPROPERTY(EditDefaultsOnly, Category="SF|Area")
	UNiagaraSystem* AreaAuraNiagara;

	//GameplayEvent 노티파이 태그
	UPROPERTY(EditDefaultsOnly, Category="SF|Area|Event")
	FGameplayTag HealEventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Skill.AreaHeal"));
	
	//AbilityTask
	UFUNCTION()
	void OnHealEventReceived(FGameplayEventData Payload);

	void StartArea();
	void UpdateAreaEffect(const FGameplayAbilityActorInfo* ActorInfo);
	void EndArea();

	//타이머
	FTimerHandle AoECheckTimer;
	FTimerHandle EndAreaTimerHandle;

	//현재 효과 적용 중 대상 + EffectHandle
	UPROPERTY()
	TMap<class ASFCharacterBase*, FActiveGameplayEffectHandle> ActiveAreaEffects;

	//FX Component
	UPROPERTY()
	TMap<class ASFCharacterBase*, class UNiagaraComponent*> ActiveAreaAuras;

	//부모 이펙트/사운드 즉시 재생 차단
	virtual void SpawnBuffVisualsAndAudio(const FGameplayAbilityActorInfo* ActorInfo) override {}

public:
	//Ability 실행
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
