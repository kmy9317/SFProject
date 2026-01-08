// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_CharacterDeath.generated.h"

class ASFEnemy;
/**
 * 
 */
UCLASS()
class SF_API USFGA_CharacterDeath : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_CharacterDeath();

	
	virtual void ActivateAbility( const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	
	virtual void DeathEventAfterDelay();
	
	virtual void DeathTimerEvent();

	// 드롭 처리
	void ExecuteDrop(ASFEnemy* Enemy);
	float GetKillerLuckValue(AActor* Killer) const;
	
	FTimerHandle EventTimerHandle;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	float EventTime = 5.f;
};
