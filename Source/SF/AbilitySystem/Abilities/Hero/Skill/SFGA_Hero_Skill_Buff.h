#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "SFGA_Hero_Skill_Buff.generated.h"

class UGameplayEffect;
class UAnimMontage;

UCLASS(Abstract, Blueprintable)
class SF_API USFGA_Hero_Skill_Buff : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Hero_Skill_Buff(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

//=========================Ability Data=========================
	UPROPERTY(EditDefaultsOnly, Category="SF|Animation")
	UAnimMontage* BuffMontage;

	UPROPERTY(EditDefaultsOnly, Category="SF|BuffEffect")
	TSubclassOf<UGameplayEffect> BuffEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="SF|BuffEffect")
	int32 BuffLevel = 1;
//==============================================================


//===================GameplayCue & Event Tags===================
	UPROPERTY(EditDefaultsOnly, Category="SF|Cue")
	FGameplayTag StartEventTag;

	UPROPERTY(EditDefaultsOnly, Category="SF|Cue")
	FGameplayTag GroundCueTag;

	UPROPERTY(EditDefaultsOnly, Category="SF|Cue")
	FGameplayTag AuraCueTag;
	
	UPROPERTY(EditDefaultsOnly, Category="SF|Cue")
	FGameplayTag SkillActivatedTag;
//==============================================================


//======================Aura Tracking===========================
	UPROPERTY()
	TMap<AActor*, FActiveGameplayEffectHandle> ActiveAuraEffects;
//==============================================================


//========================Event Handler=========================
	UFUNCTION()
	void OnReceivedSkillEvent(FGameplayEventData Payload);

	UFUNCTION(BlueprintNativeEvent)
	void OnSkillEventTriggered();
	virtual void OnSkillEventTriggered_Implementation();
//==============================================================


//========================Montage Delegate======================
	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageBlendOut();
//==============================================================


//========================Buff Handling=========================
	virtual void ApplyAura(AActor* Target);
	virtual void RemoveAura(AActor* Target);
//==============================================================

public:

//========================Ability Lifecycle======================
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;
//=============================================================
};
