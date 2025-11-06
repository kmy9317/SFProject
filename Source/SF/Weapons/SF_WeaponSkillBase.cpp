// Copyright 1998-2024 Epic Games, Inc. All Rights Reserved.

#include "Weapons/SF_WeaponSkillBase.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UWeaponSkillBase::UWeaponSkillBase()
{
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UWeaponSkillBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // 몽타주 확인
    if (!MontageToPlay)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] ActivateAbility: MontageToPlay is not set!"), *GetName());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 어빌리티 커밋 확인
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        UE_LOG(LogTemp, Log, TEXT("[%s] ActivateAbility: Failed to commit ability (e.g., not enough stamina)."), *GetName());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 몽타주 재생 태스크 생성
    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, 
        FName("PlayMontageTask"), 
        MontageToPlay, 
        1.0f,                    
        NAME_None,               
        true,                    
        1.0f,                    
        0.0f,                    
        false                    
    );

    if (MontageTask)
    {
        // 몽타주 이벤트 바인딩
        MontageTask->OnCompleted.AddDynamic(this, &UWeaponSkillBase::OnMontageCompleted);
        MontageTask->OnInterrupted.AddDynamic(this, &UWeaponSkillBase::OnMontageInterrupted);
        MontageTask->OnCancelled.AddDynamic(this, &UWeaponSkillBase::OnMontageCancelled);

        // 태스크 활성화
        MontageTask->Activate();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] ActivateAbility: Failed to create MontageTask."), *GetName());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    }
}

void UWeaponSkillBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    // 몽타주 태스크 정리
    if (MontageTask)
    {
        MontageTask->EndTask();
        MontageTask = nullptr;
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWeaponSkillBase::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWeaponSkillBase::OnMontageInterrupted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWeaponSkillBase::OnMontageCancelled()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}