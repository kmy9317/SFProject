#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Hero_Parrying.h"
#include "GameplayTagContainer.h"
#include "SFGA_Hero_Parrying_A.generated.h"

UCLASS()
class SF_API USFGA_Hero_Parrying_A : public USFGA_Hero_Parrying
{
	GENERATED_BODY()

protected:
	//Ability 종료 시 태그 제거
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,bool bReplicateEndAbility,bool bWasCancelled) override;

	virtual void OnChainMontageCompleted() override;
	virtual void OnComboStateRemoved(const FActiveGameplayEffect& RemovedEffect) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="SF|Parry")
	float ParrySuccessCooldownScale = 0.1f; //패링 성공 시 쿨타임 90% 감소

	UPROPERTY(EditDefaultsOnly, Category="SF|Parry")
	TSubclassOf<UGameplayEffect> ParrySuccessExtraEffect; //패링 성공 보상 GE

	//1회 적용 제어용 태그
	UPROPERTY(EditDefaultsOnly, Category="SF|Parry")
	FGameplayTag ParryExtraEffectAppliedTag;
};