// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGA_Groggy.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterGameplayTags.h"

USFGA_Groggy::USFGA_Groggy()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

 
    FAbilityTriggerData GroggyTrigger;
    GroggyTrigger.TriggerTag = SFGameplayTags::GameplayEvent_Groggy;
    GroggyTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(GroggyTrigger);

    ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_Groggy);
}

void USFGA_Groggy::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ASC->CancelAllAbilities(this);
    
    UAbilityTask_WaitDelay* WaitTask = UAbilityTask_WaitDelay::WaitDelay(this, GroggyDuration);
    if (WaitTask)
    {
        WaitTask->OnFinish.AddDynamic(this, &ThisClass::OnGroggyTimeFinished);
        WaitTask->ReadyForActivation();
    }
    else
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

void USFGA_Groggy::OnGroggyTimeFinished()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Groggy::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{

    if (GetAvatarActorFromActorInfo()->HasAuthority() && ResetEffect)
    {
       
        FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(ResetEffect, GetAbilityLevel());
        
        if (SpecHandle.IsValid())
        {
            ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}