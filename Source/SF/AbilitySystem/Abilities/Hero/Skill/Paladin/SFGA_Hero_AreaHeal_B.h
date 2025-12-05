#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Hero_Skill_Buff.h"
#include "SFGA_Hero_AreaHeal_B.generated.h"

class ASFCharacterBase;
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class USFGA_Hero_AreaHeal_B : public USFGA_Hero_Skill_Buff
{
	GENERATED_BODY()

public:
	USFGA_Hero_AreaHeal_B(const FObjectInitializer& ObjectInitializer);

protected:
	//스킬 장판 고정 위치
	FVector FixedOrigin;

	//반경
	UPROPERTY(EditDefaultsOnly, Category="SF|Area")
	float AreaRadius = 450.f;

	//유지 시간
	UPROPERTY(EditDefaultsOnly, Category="SF|Area")
	float AreaDuration = 6.f;

	//Tick 간격
	UPROPERTY(EditDefaultsOnly, Category="SF|Area")
	float CheckInterval = 0.5f;

	//시전자 포함 여부
	UPROPERTY(EditDefaultsOnly, Category="SF|Area")
	bool bIncludeSelf = true;

	//장판 FX
	UPROPERTY(EditDefaultsOnly, Category="SF|Area|FX")
	UNiagaraSystem* AreaNiagaraSystem = nullptr;

	UPROPERTY()
	UNiagaraComponent* AreaNiagaraComponent = nullptr;

	//무적 오라 FX
	UPROPERTY(EditDefaultsOnly, Category="SF|Area|FX")
	UNiagaraSystem* InvincibleAuraNiagara = nullptr;

	//GameplayEvent를 통한 스킬 발동
	UPROPERTY(EditDefaultsOnly, Category="SF|Area|Event")
	FGameplayTag HealEventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Skill.AreaHeal"));

	//Notify Tag 이벤트
	UFUNCTION()
	void OnHealEventReceived(FGameplayEventData Payload);

	//실제 스킬 실행
	void StartAreaSkill();
	void UpdateArea(const FGameplayAbilityActorInfo* ActorInfo);
	void EndAreaSkill();

	//적용 대상
	UPROPERTY()
	TMap<ASFCharacterBase*, FActiveGameplayEffectHandle> ActiveAreaEffects;

	UPROPERTY()
	TMap<ASFCharacterBase*, UNiagaraComponent*> ActiveAreaAuras;

	FTimerHandle AoECheckTimer;
	FTimerHandle EndAreaTimerHandle;

	virtual void SpawnBuffVisualsAndAudio(const FGameplayAbilityActorInfo* ActorInfo) override {}

public:
	//Ability 시작(몽타주만 재생)
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
