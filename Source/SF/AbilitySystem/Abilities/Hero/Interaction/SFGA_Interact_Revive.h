#pragma once

#include "CoreMinimal.h"
#include "SFGA_Interact_Object.h"
#include "SFGA_Interact_Revive.generated.h"

UCLASS()
class SF_API USFGA_Interact_Revive : public USFGA_Interact_Object
{
	GENERATED_BODY()

public:
	USFGA_Interact_Revive(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "SF|Revive")
	TSubclassOf<UGameplayEffect> ReviveRewardEffect;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Revive", meta = (Categories = "GameplayCue"))
	FGameplayTag ReviveSuccessGameplayCueTag;
};
