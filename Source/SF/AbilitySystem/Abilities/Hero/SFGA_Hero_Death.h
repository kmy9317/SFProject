// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_Hero_Death.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Hero_Death : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Hero_Death(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void CancelAllActiveAbilities();
	void HideWeapons();
	void DisablePlayerInput();
	void RestorePlayerInput();
	void PlayDeathMontage();

protected:
	// TODO : 사망 후 처리 (사망 UI Fade in/out, 관전 모드 등)
	void HandlePostDeath();
};
