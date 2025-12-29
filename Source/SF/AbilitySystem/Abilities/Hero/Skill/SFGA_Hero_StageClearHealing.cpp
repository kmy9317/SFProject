#include "SFGA_Hero_StageClearHealing.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"

USFGA_Hero_StageClearHealing::USFGA_Hero_StageClearHealing()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    
    AbilityTags.AddTag(SFGameplayTags::GameplayEvent_Stage_Clear);
}

void USFGA_Hero_StageClearHealing::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData
)
{
    if (!StageClearHealEffect || !ActorInfo)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    //이벤트 받으면 그냥 GE 1회 적용
    ApplyGameplayEffectToOwner(
        Handle,
        ActorInfo,
        ActivationInfo,
        StageClearHealEffect.GetDefaultObject(),
        GetAbilityLevel()
    );

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
