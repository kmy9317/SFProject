#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_Hero_Skill_Buff.generated.h"

class UParticleSystem;
class USoundBase;
class USceneComponent;
class UGameplayEffect;
class UAnimMontage;

UCLASS(Abstract, Blueprintable)
class SF_API USFGA_Hero_Skill_Buff : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Hero_Skill_Buff(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	//스킬 사용 애님 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|Buff|Animation")
	UAnimMontage* BuffMontage;
	//스킬 사용 이펙트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|Buff|Visual")
	UParticleSystem* BuffParticleSystem;
	//스킬 사용 사운드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|Buff|Audio")
	USoundBase* BuffSound;

	//적용할 버프 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|Buff|Effect")
	TSubclassOf<UGameplayEffect> BuffEffectClass;
	//GE 레벨
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SF|Buff|Effect")
	int32 BuffLevel = 1;

	//FX 통합 관리용
	UPROPERTY()
	TArray<USceneComponent*> ActiveFXComponents;

protected:
	//이펙트 & 사운드 재생
	virtual void SpawnBuffVisualsAndAudio(const FGameplayAbilityActorInfo* ActorInfo);

	//실제 버프 적용 로직
	virtual void ApplyBuffEffectToTargets(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo);

public:
	//Ability 시작
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
