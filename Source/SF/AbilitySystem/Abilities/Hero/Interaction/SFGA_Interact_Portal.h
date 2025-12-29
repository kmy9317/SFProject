#pragma once

#include "CoreMinimal.h"
#include "SFGA_Interact_Object.h"
#include "SFGA_Interact_Portal.generated.h"

class ASFPortal;
/**
 * 
 */
UCLASS()
class SF_API USFGA_Interact_Portal : public USFGA_Interact_Object
{
	GENERATED_BODY()

public:
	USFGA_Interact_Portal(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

};
