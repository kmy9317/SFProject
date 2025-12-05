#include "SFGA_EnemyHitReaction.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Attributes/SFPrimarySet.h"
#include "AbilitySystem/GameplayEffect/Enemy/EffectContext/FSFHitEffectContext.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AI/SFAIGameplayTags.h"
#include "Character/SFCharacterGameplayTags.h"
#include "GameFramework/Character.h"

USFGA_EnemyHitReaction::USFGA_EnemyHitReaction()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    bServerRespectsRemoteAbilityCancellation = true;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = SFGameplayTags::GameplayEvent_HitReaction;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
    
    ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_Hit);
}

void USFGA_EnemyHitReaction::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Character)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    float Damage = TriggerEventData ? TriggerEventData->EventMagnitude : 0.f;
    FVector AttackDir = ExtractHitDirectionFromEvent(TriggerEventData);
    FVector HitLoc   = ExtractHitLocationFromEvent(TriggerEventData);

    if (Damage <= 0.f)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    const USFPrimarySet* PrimarySet = Cast<USFPrimarySet>(
        ASC->GetAttributeSet(USFPrimarySet::StaticClass()));

    if (!PrimarySet)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    float CurrentHealth = PrimarySet->GetHealth();

    if (CurrentHealth <= 0.0f)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    UAnimMontage* SelectedMontage = (AttackDir.X < 0) ? BackHitMontage : FrontHitMontage;

    if (SelectedMontage)
    {
        MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, SelectedMontage);

        if (MontageTask)
        {
            MontageTask->OnCompleted.AddDynamic(this, &USFGA_EnemyHitReaction::OnMontageCompleted);
            MontageTask->OnInterrupted.AddDynamic(this, &USFGA_EnemyHitReaction::OnMontageInterrupted);
            MontageTask->OnCancelled.AddDynamic(this, &USFGA_EnemyHitReaction::OnMontageCancelled);
            MontageTask->ReadyForActivation();
            return;
        }
    }

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_EnemyHitReaction::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_EnemyHitReaction::OnMontageCancelled()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_EnemyHitReaction::OnMontageInterrupted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_EnemyHitReaction::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (!IsActive())
        return;

    if (MontageTask)
    {
        if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
        {
            ActorInfo->AbilitySystemComponent->CurrentMontageStop();
        }
        MontageTask->EndTask();
        MontageTask = nullptr;
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// Hit 위치/방향 추출 함수
FVector USFGA_EnemyHitReaction::ExtractHitLocationFromEvent(const FGameplayEventData* EventData) const
{
    if (!EventData) return FVector::ZeroVector;

    const FGameplayEffectContextHandle& Ctx = EventData->ContextHandle;
    if (const FSFHitEffectContext* SFContext = static_cast<const FSFHitEffectContext*>(Ctx.Get()))
    {
        return SFContext->GetHitLocation();
    }

    const FHitResult* HitResult = Ctx.GetHitResult();
    return HitResult ? HitResult->ImpactPoint : FVector::ZeroVector;
}

FVector USFGA_EnemyHitReaction::ExtractHitDirectionFromEvent(const FGameplayEventData* EventData) const
{
    if (!EventData) return FVector::ZeroVector;

    const FGameplayEffectContextHandle& Ctx = EventData->ContextHandle;

    if (const FSFHitEffectContext* SFContext = static_cast<const FSFHitEffectContext*>(Ctx.Get()))
    {
        if (!SFContext->GetAttackDirection().IsNearlyZero())
            return SFContext->GetAttackDirection();
    }

    const FHitResult* HitResult = Ctx.GetHitResult();
    if (HitResult)
    {
        AActor* Target = GetAvatarActorFromActorInfo();
        if (Target)
        {
            FVector ToTarget = (Target->GetActorLocation() - HitResult->ImpactPoint);
            ToTarget.Z = 0.f;
            return ToTarget.GetSafeNormal();
        }
    }

    return FVector::ZeroVector;
}
