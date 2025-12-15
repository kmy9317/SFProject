#pragma once

#include "CoreMinimal.h"
#include "SFGA_ChainedSkill_Melee.h"
#include "GameplayTagContainer.h"
#include "SFGA_Hero_Parrying.generated.h"

class UAnimMontage;

UCLASS()
class SF_API USFGA_Hero_Parrying : public USFGA_ChainedSkill_Melee
{
	GENERATED_BODY()

public:
	USFGA_Hero_Parrying(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//=========================Anim=========================
	UPROPERTY(EditDefaultsOnly, Category="SF|Animation")
	UAnimMontage* ParryMontage;	// 패링 모션
	//======================================================

	//======================Gameplay Tags===================
	// PostGameplayEffectExecute → ProcessParryEvent() → ASC.HandleGameplayEvent(ParryEventTag)
	UPROPERTY(EditDefaultsOnly, Category="SF|Parry")
	FGameplayTag ParryEventTag;
	//======================================================

	//=======================Event Handler===================
	UFUNCTION()
	virtual void OnParryEventReceived(FGameplayEventData Payload); // 패링 성공 이벤트 수신
	//=======================================================

	//======================Montage Delegate=================
	virtual void OnChainMontageCompleted() override;
	virtual void OnChainMontageInterrupted() override;
	//=======================================================

	//======================Parry State======================
	//패링 GC가 이미 실행됐는지 여부
	bool bParryGCExecuted = false;
	//======================================================

public:

	//========================Lifecycle======================
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
	//=======================================================
};
