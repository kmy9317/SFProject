// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_Hero_Downed.generated.h"

class ASFHero;
class USFPlayerCombatStateComponent;
/**
 * 다운 상태를 관리하는 어빌리티
 * - Health 0 시 활성화
 * - ReviveGauge 감소/증가 처리
 * - 게이지 0 → Death, 100 → Revive
 */
UCLASS()
class SF_API USFGA_Hero_Downed : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Hero_Downed(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void UpdateReviveGauge();
	void SetReviveGauge(float NewGauge);
	
	void HandleDeath();
	void HandleRevive();

	void DisablePlayerInput();
	void RestorePlayerInput();
	
	void PlayDownedMontage();

protected:
	
	// 초당 게이지 감소율
	UPROPERTY(EditDefaultsOnly, Category = "SF|Downed", meta = (ClampMin = "0.0"))
	float DrainRatePerSecond = 3.3f;

	// 부활자 1명당 초당 게이지 증가율 
	UPROPERTY(EditDefaultsOnly, Category = "SF|Downed", meta = (ClampMin = "0.0"))
	float FillRatePerReviver = 10.f;

	// 게이지 업데이트 간격
	UPROPERTY(EditDefaultsOnly, Category = "SF|Downed", meta = (ClampMin = "0.0"))
	float UpdateInterval = 0.1f;

	// 부활 시 회복할 Health 비율 (0.0 ~ 1.0) 
	UPROPERTY(EditDefaultsOnly, Category = "SF|Downed", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReviveHealthPercent = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Downed", meta = (ClampMin = "0.0"))
	float MaxReviveGauge = 100.f;

private:

	FTimerHandle GaugeTickTimerHandle;

	UPROPERTY()
	TWeakObjectPtr<ASFHero> CachedDownedHero;

	UPROPERTY()
	TWeakObjectPtr<USFPlayerCombatStateComponent> CachedCombatStateComponent;
	
};
