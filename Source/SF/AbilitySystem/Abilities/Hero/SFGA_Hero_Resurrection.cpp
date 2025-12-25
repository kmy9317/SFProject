#include "SFGA_Hero_Resurrection.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "Character/SFCharacterGameplayTags.h"

USFGA_Hero_Resurrection::USFGA_Hero_Resurrection(const FObjectInitializer& ObjectInitializer)
    : Super( ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void USFGA_Hero_Resurrection::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!HasAuthority(&ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // Dead/Downed 어빌리티 캔슬
    FGameplayTagContainer AbilitiesToCancel;
    AbilitiesToCancel.AddTag(SFGameplayTags::Ability_Hero_Death);
    AbilitiesToCancel.AddTag(SFGameplayTags::Ability_Hero_Downed);
    ASC->CancelAbilities(&AbilitiesToCancel);

    // 리소스 회복 (Health, Stamina, Mana → Max값으로)
    if (ResurrectionRestoreEffect)
    {
        FGameplayEffectSpecHandle RestoreSpec = MakeOutgoingGameplayEffectSpec(ResurrectionRestoreEffect, GetAbilityLevel());
        if (RestoreSpec.IsValid())
        {
            ASC->ApplyGameplayEffectSpecToSelf(*RestoreSpec.Data.Get());
        }
    }

    // 부활 이펙트 cue
    if (ResurrectionCueTag.IsValid())
    {
        ASC->ExecuteGameplayCue(ResurrectionCueTag);
    }

    // 부활 버프 (무적 시간 등)
    if (ResurrectionBuffEffect)
    {
        FGameplayEffectSpecHandle BuffSpec = MakeOutgoingGameplayEffectSpec(ResurrectionBuffEffect, GetAbilityLevel());
        if (BuffSpec.IsValid())
        {
            ASC->ApplyGameplayEffectSpecToSelf(*BuffSpec.Data.Get());
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}