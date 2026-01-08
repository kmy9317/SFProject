#include "SFGA_Dragon_FlameBreath_Line.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/SFCharacterBase.h"
#include "DrawDebugHelpers.h"
#include "AbilitySystem/GameplayCues/SFGameplayCueTags.h"
#include "AbilitySystem/GameplayCues/Data/SFGameplayCueCosmeticData.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "Interface/SFAIControllerInterface.h"
#include "Animation/AnimInstance.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"
#include "Engine/OverlapResult.h"
#include "Kismet/KismetMathLibrary.h"

USFGA_Dragon_FlameBreath_Line::USFGA_Dragon_FlameBreath_Line()
{
    AbilityID = FName("Dragon_FlameBreath_Line");
    AttackType = EAttackType::Range;

    AbilityTags.AddTag(SFGameplayTags::Ability_Dragon_FlameBreath_Line);
    CoolDownTag = SFGameplayTags::Ability_Cooldown_Dragon_FlameBreath_Line;
}

void USFGA_Dragon_FlameBreath_Line::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    StartCharging();
}

void USFGA_Dragon_FlameBreath_Line::StartCharging()
{
    AccumulatedInterruptDamage = 0.f;
    HitActors.Empty();

    PrimaryTarget = FindPrimaryTarget();
    if (!PrimaryTarget.IsValid())
    {
       EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
       return;
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            RotationTimerHandle,
            this,
            &USFGA_Dragon_FlameBreath_Line::UpdateRotationToTarget,
            0.01f, 
            true
        );
    }

    if (BreathMontage)
    {
       ChargeStartMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
          this,
          NAME_None,
          BreathMontage,
          1.f,
          FName("ChargeStart"),
          true
       );

       if (ChargeStartMontageTask)
       {
          ChargeStartMontageTask->OnCompleted.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnChargeStartCompleted);
          ChargeStartMontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnMontageInterrupted);
          ChargeStartMontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnMontageCancelled);
          ChargeStartMontageTask->ReadyForActivation();
       }
    }

    if (CurrentActorInfo->IsNetAuthority())
    {
       UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
       if (ASC)
       {
          OnDamageReceivedHandle = ASC->OnGameplayEffectAppliedDelegateToSelf.AddUObject(
             this,
             &USFGA_Dragon_FlameBreath_Line::OnDamageReceivedDuringCharge
          );
       }
    }

    if (GetWorld())
    {
       GetWorld()->GetTimerManager().SetTimer(
          ChargeTimerHandle,
          this,
          &USFGA_Dragon_FlameBreath_Line::TransitionToBreath,
          ChargeDuration,
          false
       );
    }
}

void USFGA_Dragon_FlameBreath_Line::UpdateRotationToTarget()
{
    if (!PrimaryTarget.IsValid()) return;
    
    ACharacter* Dragon = GetSFCharacterFromActorInfo();
    if (!Dragon) return;

    FVector MyLoc = Dragon->GetActorLocation();
    FVector TargetLoc = PrimaryTarget->GetActorLocation();

    TargetLoc.Z = MyLoc.Z;

    FRotator TargetRot = UKismetMathLibrary::FindLookAtRotation(MyLoc, TargetLoc);
    FRotator CurrentRot = Dragon->GetActorRotation();

    FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, 0.01f, 0.7f);
    
    Dragon->SetActorRotation(NewRot);
}

void USFGA_Dragon_FlameBreath_Line::OnChargeStartCompleted()
{
    ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
    if (Dragon && Dragon->GetMesh())
    {
       UAnimInstance* AnimInst = Dragon->GetMesh()->GetAnimInstance();
       if (AnimInst && BreathMontage)
       {
          AnimInst->Montage_JumpToSection(FName("ChargeLoop"), BreathMontage);
       }
    }
}

void USFGA_Dragon_FlameBreath_Line::TransitionToBreath()
{

    if (OnDamageReceivedHandle.IsValid())
    {
       UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
       if (ASC)
       {
          ASC->OnGameplayEffectAppliedDelegateToSelf.Remove(OnDamageReceivedHandle);
          OnDamageReceivedHandle.Reset();
       }
    }

    ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
    if (Dragon && Dragon->GetMesh())
    {
       UAnimInstance* AnimInst = Dragon->GetMesh()->GetAnimInstance();
       if (AnimInst && BreathMontage)
       {
          AnimInst->Montage_SetNextSection(
             FName("ChargeLoop"),
             FName("BreathStart"),
             BreathMontage
          );
          AnimInst->Montage_JumpToSection(FName("BreathStart"), BreathMontage);
       }
    }
    
    if (Dragon)
    {
        FGameplayCueParameters Params;
        Params.Location = Dragon->GetActorLocation();
        Params.EffectCauser = Dragon;
        Params.Instigator = Dragon;
        
        FireGameplayCueWithCosmetic_Actor(SFGameplayTags::GameplayCue_Dragon_Breath_Line_Fire, Params);
        FireGameplayCueWithCosmetic_Actor(SFGameplayTags::GameplayCue_Dragon_Breath_Line_Ground, Params);
    }

    if (!CurrentActorInfo->IsNetAuthority())
    {
       return;
    }

    if (GetWorld())
    {
       GetWorld()->GetTimerManager().SetTimer(
          BreathTickTimer,
          this,
          &USFGA_Dragon_FlameBreath_Line::ApplyBreathDamage,
          BreathTickRate,
          true
       );

       GetWorld()->GetTimerManager().SetTimer(
          BreathDurationTimer,
          this,
          &USFGA_Dragon_FlameBreath_Line::StopBreath,
          BreathDuration,
          false
       );
    }
}

void USFGA_Dragon_FlameBreath_Line::ApplyBreathDamage()
{
    ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
    if (!Dragon) return;

    USkeletalMeshComponent* Mesh = Dragon->GetMesh();
    if (!Mesh) return;
	
    const FVector JawLoc = Mesh->GetSocketLocation(TEXT("JawSocket"));
    const FVector Forward = Dragon->GetActorForwardVector();
    
    const float Range = BreathRange;          // 브레스 사거리 
    const float ConeHalfAngle = 30.0f;        // 부채꼴 절반 각도 
    const float BodyRadius = 800.0f;          // 몸통 열기 범위 
    const float MuzzleRadius = 300.0f;        // 입 바로 앞 범위 

 
    const float CosThreshold = FMath::Cos(FMath::DegreesToRadians(ConeHalfAngle));
	
    if (bIsDebug)
    {
        // 1. 브레스 부채꼴
        DrawDebugCone(GetWorld(), JawLoc, Forward, Range, 
            FMath::DegreesToRadians(ConeHalfAngle), FMath::DegreesToRadians(ConeHalfAngle), 
            16, FColor::Red, false, BreathTickRate, 0, 2.0f);
            
        // 2. 몸통 범위
        DrawDebugSphere(GetWorld(), Dragon->GetActorLocation(), BodyRadius, 16, FColor::Orange, false, BreathTickRate);
        
        // 3. 입 앞 범위
        DrawDebugSphere(GetWorld(), JawLoc, MuzzleRadius, 16, FColor::Yellow, false, BreathTickRate);
    }


    TArray<FOverlapResult> Overlaps;
    FCollisionShape BigSphere = FCollisionShape::MakeSphere(Range);
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Dragon);


    bool bHasOverlap = GetWorld()->OverlapMultiByChannel(
        Overlaps, JawLoc, FQuat::Identity, ECC_Pawn, BigSphere, QueryParams
    );
    

    TArray<FOverlapResult> BodyOverlaps;
    GetWorld()->OverlapMultiByChannel(
        BodyOverlaps, Dragon->GetActorLocation(), FQuat::Identity, ECC_Pawn, 
        FCollisionShape::MakeSphere(BodyRadius), QueryParams
    );
    Overlaps.Append(BodyOverlaps);
	
    TSet<AActor*> ProcessedVictims;

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Victim = Overlap.GetActor();
        

        if (!Victim || Victim == Dragon || ProcessedVictims.Contains(Victim)) continue;
        if (GetAttitudeTowards(Victim) != ETeamAttitude::Hostile) continue;

        bool bShouldHit = false;
        FVector VictimLoc = Victim->GetActorLocation();


        float DistToBody = FVector::Dist(Dragon->GetActorLocation(), VictimLoc);
        if (DistToBody <= BodyRadius)
        {
            bShouldHit = true; 
        }
    
        else if (FVector::Dist(JawLoc, VictimLoc) <= MuzzleRadius)
        {
            bShouldHit = true;
        }
      
        else
        {
            FVector ToVictim = (VictimLoc - JawLoc).GetSafeNormal();
            float Dot = FVector::DotProduct(Forward, ToVictim);
        	
            if (Dot >= CosThreshold && FVector::Dist(JawLoc, VictimLoc) <= Range)
            {
                bShouldHit = true;
            }
        }


        if (bShouldHit)
        {

            if (DistToBody > BodyRadius)
            {
                FHitResult Hit;
                bool bBlocked = GetWorld()->LineTraceSingleByChannel(
                    Hit, JawLoc, VictimLoc, ECC_Visibility, QueryParams);
                
                if (bBlocked && Hit.GetActor() != Victim) continue; 
            }

         
            ProcessedVictims.Add(Victim); 
            
            FGameplayEffectContextHandle EffectContext = MakeEffectContext(CurrentSpecHandle, CurrentActorInfo);
            ApplyRawDamageToTarget(Victim, BreathDamagePerTick, EffectContext);
            ApplyPressureToTarget(Victim);
        }
    }
}

void USFGA_Dragon_FlameBreath_Line::StopBreath()
{
    if (GetWorld())
    {
       GetWorld()->GetTimerManager().ClearTimer(BreathTickTimer);
    }
    
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->RemoveGameplayCue(SFGameplayTags::GameplayCue_Dragon_Breath_Line_Fire);
        ASC->RemoveGameplayCue(SFGameplayTags::GameplayCue_Dragon_Breath_Line_Ground);
    }
    
    if (ChargeStartMontageTask)
    {
       ChargeStartMontageTask->OnCompleted.RemoveAll(this);
       ChargeStartMontageTask->OnInterrupted.RemoveAll(this);
       ChargeStartMontageTask->OnCancelled.RemoveAll(this);
       ChargeStartMontageTask->EndTask();
       ChargeStartMontageTask = nullptr;
    }

    ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
    if (Dragon && Dragon->GetMesh())
    {
       UAnimInstance* AnimInst = Dragon->GetMesh()->GetAnimInstance();
       if (AnimInst && BreathMontage)
       {
          AnimInst->Montage_SetNextSection(
             FName("BreathLoop"),
             FName("BreathEnd"),
             BreathMontage
          );

          AnimInst->Montage_JumpToSection(FName("BreathEnd"), BreathMontage);
       }
    }
    
    if (BreathMontage)
    {
       BreathEndMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
          this,
          NAME_None,
          BreathMontage,
          1.f,
          FName("BreathEnd"),
          false
       );

       if (BreathEndMontageTask)
       {
          BreathEndMontageTask->OnCompleted.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnBreathEndCompleted);
          BreathEndMontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnMontageInterrupted);
          BreathEndMontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_FlameBreath_Line::OnMontageCancelled);
          BreathEndMontageTask->ReadyForActivation();
       }
    }
}

AActor* USFGA_Dragon_FlameBreath_Line::FindPrimaryTarget()
{
    AController* Controller = GetControllerFromActorInfo();
    if (!Controller) return nullptr;

    APawn* Pawn = Controller->GetPawn();
    if (!Pawn) return nullptr;

    ISFAIControllerInterface* AIC = Cast<ISFAIControllerInterface>(Pawn->GetController());
    if (!AIC) return nullptr;

    USFCombatComponentBase* CombatComp = AIC->GetCombatComponent();
    if (!CombatComp) return nullptr;

    return CombatComp->GetCurrentTarget();
}

void USFGA_Dragon_FlameBreath_Line::OnDamageReceivedDuringCharge(UAbilitySystemComponent* Source, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle)
{
    float Damage = SpecApplied.GetSetByCallerMagnitude(SFGameplayTags::Data_Damage_BaseDamage, false, 0.f);

    if (Damage > 0.f)
    {
       AccumulatedInterruptDamage += Damage;

       if (AccumulatedInterruptDamage >= InterruptThreshold)
       {
          InterruptBreath();
       }
    }
}

void USFGA_Dragon_FlameBreath_Line::InterruptBreath()
{
    if (GetWorld())
    {
       GetWorld()->GetTimerManager().ClearTimer(ChargeTimerHandle);
       GetWorld()->GetTimerManager().ClearTimer(RotationTimerHandle);
    }

    if (OnDamageReceivedHandle.IsValid())
    {
       UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
       if (ASC)
       {
          ASC->OnGameplayEffectAppliedDelegateToSelf.Remove(OnDamageReceivedHandle);
          OnDamageReceivedHandle.Reset();
       }
    }

    if (InterruptStaggerEffect)
    {
       UAbilitySystemComponent* DragonASC = GetAbilitySystemComponentFromActorInfo();
       if (DragonASC)
       {
          FGameplayEffectSpecHandle StaggerSpec = MakeOutgoingGameplayEffectSpec(InterruptStaggerEffect, 1);

          if (StaggerSpec.IsValid())
          {
             StaggerSpec.Data->SetSetByCallerMagnitude(
                SFGameplayTags::Data_Stagger_BaseStagger,
                InterruptStaggerDamage
             );

             DragonASC->ApplyGameplayEffectSpecToSelf(*StaggerSpec.Data.Get());
          }
       }
    }

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Dragon_FlameBreath_Line::OnBreathEndCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Dragon_FlameBreath_Line::OnMontageInterrupted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Dragon_FlameBreath_Line::OnMontageCancelled()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Dragon_FlameBreath_Line::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (GetWorld())
    {
       GetWorld()->GetTimerManager().ClearTimer(ChargeTimerHandle);
       GetWorld()->GetTimerManager().ClearTimer(BreathTickTimer);
       GetWorld()->GetTimerManager().ClearTimer(BreathDurationTimer);
       GetWorld()->GetTimerManager().ClearTimer(RotationTimerHandle);
    }

    if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
    {
        ActorInfo->AbilitySystemComponent->RemoveGameplayCue(SFGameplayTags::GameplayCue_Dragon_Breath_Line_Fire);
        ActorInfo->AbilitySystemComponent->RemoveGameplayCue(SFGameplayTags::GameplayCue_Dragon_Breath_Line_Ground);
    }

    if (ChargeStartMontageTask)
    {
       ChargeStartMontageTask->EndTask();
       ChargeStartMontageTask = nullptr;
    }

    if (BreathEndMontageTask)
    {
       BreathEndMontageTask->EndTask();
       BreathEndMontageTask = nullptr;
    }

    if (OnDamageReceivedHandle.IsValid())
    {
       UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
       if (ASC)
       {
          ASC->OnGameplayEffectAppliedDelegateToSelf.Remove(OnDamageReceivedHandle);
          OnDamageReceivedHandle.Reset();
       }
    }

    PrimaryTarget.Reset();
    HitActors.Empty();
    AccumulatedInterruptDamage = 0.f;

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

float USFGA_Dragon_FlameBreath_Line::CalcScoreModifier(const FEnemyAbilitySelectContext& Context) const
{
    const FBossEnemyAbilitySelectContext* BossContext =
       static_cast<const FBossEnemyAbilitySelectContext*>(&Context);

    if (Context.DistanceToTarget < 1000.f)
    {
        return -10000.f; 
    }

    float Modifier = 0.f;

    if (BossContext && BossContext->Zone == EBossAttackZone::Mid)
    {
       Modifier += 800.f;
    }

    if (BossContext && BossContext->Zone == EBossAttackZone::Long)
    {
       Modifier += 600.f;
    }

    if (Context.DistanceToTarget > 2000.f)
    {
       Modifier += 500.f;
    }

    if (!Context.Target)
       return Modifier;

    UAbilitySystemComponent* TargetASC =
       UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Context.Target);

    if (!TargetASC)
       return Modifier;

    if (!TargetASC->HasMatchingGameplayTag(SFGameplayTags::Dragon_Pressure_Forward) &&
       !TargetASC->HasMatchingGameplayTag(SFGameplayTags::Dragon_Pressure_Back))
    {
       Modifier += 300.f;
    }

    if (TargetASC->HasMatchingGameplayTag(SFGameplayTags::Dragon_Pressure_Back))
    {
       Modifier -= 400.f;
    }

    return FMath::Max(Modifier, 0.f);
}