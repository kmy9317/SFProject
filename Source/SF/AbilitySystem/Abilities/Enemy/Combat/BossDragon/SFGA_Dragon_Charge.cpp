// SFGA_Dragon_Charge.cpp

#include "SFGA_Dragon_Charge.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "AbilitySystem/GameplayCues/SFGameplayCueTags.h"
#include "Character/SFCharacterBase.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Interface/SFEnemyAbilityInterface.h"
#include "AI/Controller/Dragon/SFDragonCombatComponent.h"


USFGA_Dragon_Charge::USFGA_Dragon_Charge()
{
    AbilityID = FName("Dragon_Charge");
    AttackType = EAttackType::Melee;

    AbilityTags.AddTag(SFGameplayTags::Ability_Dragon_Charge);
    CoolDownTag = SFGameplayTags::Ability_Cooldown_Dragon_Charge;
}

void USFGA_Dragon_Charge::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
    AActor* Target = GetCurrentTarget(); 

    if (!Dragon)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
    {
        FGameplayCueParameters CueParams;
        CueParams.Location = Dragon->GetActorLocation();
        CueParams.EffectCauser = Dragon;
        CueParams.Instigator = Dragon;
        ASC->AddGameplayCue(SFGameplayTags::GameplayCue_Dragon_Charge, CueParams);
    }
 
    if (ChargeMontage)
    {
        MontageTaskRef = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, ChargeMontage, 1.0f, NAME_None, true, 1.0f, 0.0f
        );

        if (MontageTaskRef)
        {
            MontageTaskRef->OnInterrupted.AddDynamic(this, &USFGA_Dragon_Charge::OnMontageEnded);
            MontageTaskRef->OnCancelled.AddDynamic(this, &USFGA_Dragon_Charge::OnMontageEnded);
            MontageTaskRef->ReadyForActivation();
        }
    }

    if (UCapsuleComponent* Capsule = Dragon->GetCapsuleComponent())
    {
        OriginalPawnResponse = Capsule->GetCollisionResponseToChannel(ECC_Pawn);
        Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
        Capsule->SetGenerateOverlapEvents(true);

        if (ActorInfo->IsNetAuthority())
        {
            Capsule->OnComponentBeginOverlap.AddDynamic(this, &USFGA_Dragon_Charge::OnChargeOverlap);
        }
    }

    if (!ActorInfo->IsNetAuthority())
    {
        return;
    }
    
    FVector ChargeDir = Dragon->GetActorForwardVector();
    if (Target)
    {
        ChargeDir = (Target->GetActorLocation() - Dragon->GetActorLocation()).GetSafeNormal2D();
    }

    UAbilityTask_ApplyRootMotionConstantForce* ForceTask =
        UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
            this,
            FName("DragonChargeForce"),
            ChargeDir,
            ChargeSpeed,
            ChargeDuration,   
            false,
            nullptr,
            ERootMotionFinishVelocityMode::SetVelocity,
            FVector::ZeroVector,
            0.f,
            true
        );

    if (ForceTask)
    {
        ForceTask->OnFinish.AddDynamic(this, &USFGA_Dragon_Charge::OnChargeFinished);
        ForceTask->ReadyForActivation();
    }
}

void USFGA_Dragon_Charge::OnChargeFinished()
{
    if (MontageTaskRef && MontageTaskRef->IsActive())
    {
        MontageTaskRef->ExternalCancel();
    }

    FinishCharge(false);
}

void USFGA_Dragon_Charge::OnMontageEnded()
{
    FinishCharge(false);
}

void USFGA_Dragon_Charge::FinishCharge(bool bCancelled)
{
    if (!IsActive())
        return;

    ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
    
    if (Dragon && Dragon->GetCharacterMovement())
    {
        Dragon->GetCharacterMovement()->StopMovementImmediately();
    }
    
    if (MontageTaskRef && MontageTaskRef->IsActive())
    {
        MontageTaskRef->EndTask();
    }
    MontageTaskRef = nullptr;

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bCancelled);
}

void USFGA_Dragon_Charge::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();

    if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
    {
        ActorInfo->AbilitySystemComponent->RemoveGameplayCue(SFGameplayTags::GameplayCue_Dragon_Charge);
    }
    
    if (Dragon)
    {
        if (UCapsuleComponent* Capsule = Dragon->GetCapsuleComponent())
        {
            Capsule->OnComponentBeginOverlap.RemoveAll(this);
            Capsule->SetCollisionResponseToChannel(ECC_Pawn, OriginalPawnResponse);
        }
    }

    MontageTaskRef = nullptr;
    HitActors.Empty();

    CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, true);

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool USFGA_Dragon_Charge::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
        return false;
    
    if (ActorInfo->IsNetAuthority())
    {
        AActor* Target = GetCurrentTarget();
        return Target != nullptr;
    }

    return true;
}

void USFGA_Dragon_Charge::OnChargeOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == GetAvatarActorFromActorInfo() || HitActors.Contains(OtherActor))
        return;
    
    if (GetAttitudeTowards(OtherActor) != ETeamAttitude::Hostile)
        return;

    ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
    if (Dragon)
    {
        ApplyDamageToTarget(OtherActor, MakeEffectContext(CurrentSpecHandle, CurrentActorInfo));
        ApplyKnockBackToTarget(OtherActor, Dragon->GetActorLocation());
        HitActors.Add(OtherActor);
    }
}

float USFGA_Dragon_Charge::CalcScoreModifier(const FEnemyAbilitySelectContext& Context) const
{
    if (Context.DistanceToTarget < 1500.f)
    {
        return -10000.f;
    }

    if (Context.Self && Context.Target)
    {
        FVector OwnerLoc = Context.Self->GetActorLocation();
        FVector TargetLoc = Context.Target->GetActorLocation();
        OwnerLoc.Z = 0.f;
        TargetLoc.Z = 0.f;

        FVector ToTarget = (TargetLoc - OwnerLoc).GetSafeNormal();
        FVector OwnerForward = Context.Self->GetActorForwardVector();
        OwnerForward.Z = 0.f;
        OwnerForward.Normalize();

        float DotResult = FVector::DotProduct(OwnerForward, ToTarget);
        
        if (DotResult < 0.8f)
        {
            return -5000.f;
        }
    }

    const FBossEnemyAbilitySelectContext* BossContext =
        static_cast<const FBossEnemyAbilitySelectContext*>(&Context);

    float Modifier = 0.f;
    
    if (BossContext && BossContext->Zone == EBossAttackZone::OutOfRange)
    {
        Modifier += 2000.f;
    }

    if (BossContext && BossContext->Zone == EBossAttackZone::Long)
    {
        Modifier += 1500.f;
    }

    if (BossContext && BossContext->Zone == EBossAttackZone::Mid)
    {
        Modifier += 500.f;
    }

    if (Context.DistanceToTarget > 5000.f)
    {
        Modifier += 1000.f;
    }

    if (!Context.Target)
        return Modifier;

    UAbilitySystemComponent* TargetASC =
        UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Context.Target);

    if (!TargetASC)
        return Modifier;
    
    if (TargetASC->HasMatchingGameplayTag(SFGameplayTags::Dragon_Pressure_Back))
    {
        Modifier += 500.f;
    }

    return Modifier;
}