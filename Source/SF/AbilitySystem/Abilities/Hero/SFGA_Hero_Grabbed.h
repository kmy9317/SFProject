#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_Hero_Grabbed.generated.h"

class UAbilityTask_WaitGameplayEvent;
class UAbilityTask_PlayMontageAndWait;
/**
 * 
 */
UCLASS()
class SF_API USFGA_Hero_Grabbed : public USFGameplayAbility
{
	GENERATED_BODY()
public:
	USFGA_Hero_Grabbed(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void DisablePlayerInput();
	void RestorePlayerInput();
	void PlayGrabbedMontage();

	UFUNCTION()
	void OnReleaseEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

protected:
	// HeroAnimationData에서 몽타주 조회용 태그
	UPROPERTY(EditDefaultsOnly, Category = "SF|Grabbed", meta = (Categories = "Montage"))
	FGameplayTag MontageTag;

	// 해제 이벤트 태그
	UPROPERTY(EditDefaultsOnly, Category = "SF|Grabbed", meta = (Categories = "GameplayEvent"))
	FGameplayTag ReleaseEventTag;

private:
	UPROPERTY()
	TWeakObjectPtr<AActor> GrabberActor;
};
