// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_DeathHandler.generated.h"

class USFPlayerCombatStateComponent;

/**
 * 체력 0 이벤트 수신 어빌리티(패시브)
 */
UCLASS()
class SF_API USFGA_DeathHandler : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_DeathHandler(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnZeroHealthReceived(FGameplayEventData Payload);

private:
	// LastStand 조건 확인 및 처리, 성공 시 true 반환
	bool TryLastStand(UAbilitySystemComponent* ASC, const FGameplayEventData& Payload);

	// DBNO 조건 확인, 진입 가능하면 true 반환
	bool CanEnterDownedState() const;
};
