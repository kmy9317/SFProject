#pragma once

#include "CoreMinimal.h"
#include "SFGA_Hero_Consume.h"
#include "SFGA_Hero_DrinkPotion.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Hero_DrinkPotion : public USFGA_Hero_Consume
{
	GENERATED_BODY()

public:
	USFGA_Hero_DrinkPotion(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
private:
	UFUNCTION()
	void OnDrinkBegin(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageFinished();

protected:
	// Fragment에 몽타주 태그가 없을 경우 기본값
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (Categories = "Montage"))
	FGameplayTag DefaultDrinkMontageTag;
	
};
