#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "SFGA_Hero_Skill_Buff.generated.h"

class UAnimMontage;
class ASFBuffArea;

UCLASS(Abstract, Blueprintable)
class SF_API USFGA_Hero_Skill_Buff : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Hero_Skill_Buff(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//=====================Ability Data=====================
	//사용할 버프 몽타주
	UPROPERTY(EditDefaultsOnly, Category="SF|Animation")
	UAnimMontage* BuffMontage = nullptr;

	//GameplayEvent를 받을 태그 (노티파이에서 쏴주는 이벤트)
	UPROPERTY(EditDefaultsOnly, Category="SF|Cue")
	FGameplayTag StartEventTag;

	//이 스킬이 소환할 장판 액터 클래스 (BP_SFBuffArea 지정)
	UPROPERTY(EditDefaultsOnly, Category="SF|Area")
	TSubclassOf<ASFBuffArea> BuffAreaClass;
	//=====================================================

	//=====================Gameplay Event=====================
	//GameplayEvent 수신 콜백
	UFUNCTION()
	void OnReceivedSkillEvent(FGameplayEventData Payload);

	//실제 스킬 로직(기본 구현 = BuffArea 소환)
	UFUNCTION(BlueprintNativeEvent)
	void OnSkillEventTriggered();
	virtual void OnSkillEventTriggered_Implementation();
	//=======================================================

	//=====================Montage Delegate=====================
	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageBlendOut();
	
	UFUNCTION()
	void OnMontageCompleted();
	//=========================================================

public:

	//===================Ability Lifecycle===================
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
