#pragma once

#include "CoreMinimal.h"
#include "SFGA_Interact_Object.h"
#include "SFGA_Interact_Pickup.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Interact_Pickup : public USFGA_Interact_Object
{
	GENERATED_BODY()

public:
	USFGA_Interact_Pickup(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
