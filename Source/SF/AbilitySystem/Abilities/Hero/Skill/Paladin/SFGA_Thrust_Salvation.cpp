#include "SFGA_Thrust_Salvation.h"

void USFGA_Thrust_Salvation::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

AActor* USFGA_Thrust_Salvation::FindLowestHPAlly()
{
	return nullptr;
}
