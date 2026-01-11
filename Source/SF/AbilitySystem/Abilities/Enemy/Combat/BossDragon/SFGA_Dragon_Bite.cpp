#include "SFGA_Dragon_Bite.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GameplayCues/SFGameplayCueTags.h"
#include "AbilitySystem/GameplayCues/Data/SFGameplayCueCosmeticData.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterBase.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "Interface/SFAIControllerInterface.h"
#include "Kismet/KismetMathLibrary.h"

USFGA_Dragon_Bite::USFGA_Dragon_Bite()
{
    AbilityID = FName("Dragon_Bite");
    AttackType = EAttackType::Melee;

    AbilityTags.AddTag(SFGameplayTags::Ability_Dragon_Bite);
    ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_UsingAbility);
    CoolDownTag = SFGameplayTags::Ability_Cooldown_Dragon_Bite;
}

void USFGA_Dragon_Bite::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CurrentBiteCount = 0;
    CurrentHitCount = 0;
    LastDamageTime = -999.0f;

    // 타겟 설정
    PrimaryTarget = FindPrimaryTarget();

    StartBiteAttack();

    if (!ActorInfo->IsNetAuthority())
    {
       return;
    }

    UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this,SFGameplayTags::GameplayEvent_Tracing,nullptr,true,true);
    if (WaitEventTask)
    {
       WaitEventTask->EventReceived.AddDynamic(this, &USFGA_Dragon_Bite::OnBiteHit);
       WaitEventTask->ReadyForActivation();
    }
}

void USFGA_Dragon_Bite::StartBiteAttack()
{
    ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
    if (!Dragon || !Dragon->GetMesh() || !BiteMontage)
    {
       EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
       return;
    }

    UAnimInstance* AnimInst = Dragon->GetMesh()->GetAnimInstance();
    if (!AnimInst)
    {
       EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
       return;
    }
   
    if (GetWorld() && PrimaryTarget.IsValid())
    {
        GetWorld()->GetTimerManager().SetTimer(
            RotationTimerHandle,
            this,
            &USFGA_Dragon_Bite::UpdateRotationToTarget,
            0.01f, 
            true
        );
    }

    UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
       this,
       NAME_None,
       BiteMontage,
       1.0f,
       FName("BiteStart"),
       true
    );

    if (MontageTask)
    {
       MontageTask->OnCompleted.AddDynamic(this, &USFGA_Dragon_Bite::OnBiteMontageCompleted);
       MontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_Bite::OnMontageInterrupted);
       MontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_Bite::OnMontageCancelled);
       MontageTask->ReadyForActivation();
    }
}

void USFGA_Dragon_Bite::UpdateRotationToTarget()
{
    if (!PrimaryTarget.IsValid()) return;
    
    ACharacter* Dragon = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Dragon) return;

    FVector MyLoc = Dragon->GetActorLocation();
    FVector TargetLoc = PrimaryTarget->GetActorLocation();
    TargetLoc.Z = MyLoc.Z;

    FRotator TargetRot = UKismetMathLibrary::FindLookAtRotation(MyLoc, TargetLoc);
    FRotator CurrentRot = Dragon->GetActorRotation();

    // 물기는 근접이라 빠르게 반응해야 하므로 RotationSpeed(10.0f) 사용
    FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, 0.01f, RotationSpeed);
    
    Dragon->SetActorRotation(NewRot);
}

void USFGA_Dragon_Bite::OnBiteMontageCompleted()
{
    if (GrabbedTarget.IsValid())
    {
       PlayGrabMontage();
       return;
    }

    CurrentBiteCount++;

    if (CurrentBiteCount < BiteCount)
    {
       PlayBiteLoop();
    }
    else
    {
       EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

void USFGA_Dragon_Bite::PlayBiteLoop()
{
    ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
    if (!Dragon || !Dragon->GetMesh() || !BiteMontage)
    {
       EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
       return;
    }

    UAnimInstance* AnimInst = Dragon->GetMesh()->GetAnimInstance();
    if (!AnimInst)
    {
       EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
       return;
    }

    UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
       this,
       NAME_None,
       BiteMontage,
       1.0f,
       FName("BiteLoop"),
       false
    );

    if (MontageTask)
    {
       MontageTask->OnCompleted.AddDynamic(this, &USFGA_Dragon_Bite::OnBiteMontageCompleted);
       MontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_Bite::OnMontageInterrupted);
       MontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_Bite::OnMontageCancelled);
       MontageTask->ReadyForActivation();
    }
}

void USFGA_Dragon_Bite::OnGrabMontageCompleted()
{
    if (GrabbedTarget.IsValid())
    {
       ApplyExecutionDamage(GrabbedTarget.Get());
    }

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Dragon_Bite::OnMontageInterrupted()
{
    if (GrabbedTarget.IsValid())
    {
       return;
    }
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Dragon_Bite::OnMontageCancelled()
{
    if (GrabbedTarget.IsValid())
    {
       return;
    }
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_Dragon_Bite::OnBiteHit(FGameplayEventData Payload)
{
    // 물기에 성공했으므로 회전 정지
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(RotationTimerHandle);
    }

    const FHitResult* HitResult = Payload.ContextHandle.GetHitResult();
    if (!HitResult) return;
    
    AActor* HitActor = HitResult->GetActor();
    if (!HitActor) return;

    if (GetAttitudeTowards(HitActor) != ETeamAttitude::Hostile)
       return;

    if (GrabbedTarget.IsValid())
    {
       return;
    }

    FGameplayEffectContextHandle EffectContext = MakeEffectContext(CurrentSpecHandle, CurrentActorInfo);
    EffectContext.AddHitResult(*HitResult);

    ApplyDamageToTarget(HitActor, EffectContext);
    TriggerGrabAbilityOnTarget(HitActor);
    AttachTargetToJaw(HitActor);
    GrabbedTarget = HitActor;
    ApplyPressureToTarget(HitActor);

    // 구출 시스템 델리게이트 등록
    CurrentHitCount = 0;
    LastDamageTime = -999.0f;

    OnDamageRecivedHandle = GetAbilitySystemComponentFromActorInfo()
       ->OnGameplayEffectAppliedDelegateToSelf.AddUObject(
           this,
           &ThisClass::OnDamageRecieved
       );

    // 물린 상태 루프 GameplayCue
    FGameplayCueParameters Params;
    Params.Location = HitActor->GetActorLocation();
    Params.EffectCauser = GetAvatarActorFromActorInfo();
    Params.Instigator = GetAvatarActorFromActorInfo();
    FireGameplayCueWithCosmetic_Actor(SFGameplayTags::GameplayCue_Dragon_Bite_Loop, Params);
   
    PlayGrabMontage();
}

void USFGA_Dragon_Bite::TriggerGrabAbilityOnTarget(AActor* Target)
{
   if (!Target)
   {
      return;
   }

   UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
   if (!TargetASC)
   {
      return;
   }

   FGameplayEventData EventData;
   EventData.EventTag = SFGameplayTags::GameplayEvent_Grabbed;
   EventData.Instigator = GetAvatarActorFromActorInfo();
   EventData.Target = Target;
   FGameplayEffectContextHandle Context = MakeEffectContext(CurrentSpecHandle, CurrentActorInfo);
   EventData.ContextHandle = Context;
   TargetASC->HandleGameplayEvent(SFGameplayTags::GameplayEvent_Grabbed, &EventData);
}

void USFGA_Dragon_Bite::SendReleaseEvent(AActor* Target)
{
   if (!Target)
   {
      return;
   }

   UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
   if (TargetASC)
   {
      FGameplayEventData EventData;
      EventData.EventTag = SFGameplayTags::GameplayEvent_GrabRelease;
      EventData.Instigator = GetAvatarActorFromActorInfo();
      EventData.Target = Target;
      TargetASC->HandleGameplayEvent(SFGameplayTags::GameplayEvent_GrabRelease, &EventData);
   }
}

void USFGA_Dragon_Bite::AttachTargetToJaw(AActor* Target)
{
    if (!Target)
        return;

    ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
    if (!Dragon)
        return;

    USkeletalMeshComponent* DragonMesh = Dragon->GetMesh();
    if (!DragonMesh)
        return;

    if (ACharacter* Char = Cast<ACharacter>(Target))
    {
       Char->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    Target->AttachToComponent(
     DragonMesh,
     FAttachmentTransformRules::SnapToTargetNotIncludingScale,
     JawSocketName);
}

void USFGA_Dragon_Bite::DetachTarget(AActor* Target)
{
    if (!Target)
    {
       return;
    }
   
    ACharacter* Char = Cast<ACharacter>(Target);
    Target->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
   
    if (Char)
    {
       UCapsuleComponent* Capsule = Char->GetCapsuleComponent();
       Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
       Char->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    }

    GetAbilitySystemComponentFromActorInfo()->RemoveGameplayCue(SFGameplayTags::GameplayCue_Dragon_Bite_Loop);
}

void USFGA_Dragon_Bite::PlayGrabMontage()
{
    ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
    if (!Dragon || !Dragon->GetMesh() || !BiteMontage)
    {
       EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
       return;
    }

    UAnimInstance* AnimInst = Dragon->GetMesh()->GetAnimInstance();
    if (!AnimInst)
    {
       EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
       return;
    }
    
    UAbilityTask_PlayMontageAndWait* GrabMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
       this,
       NAME_None,
       BiteMontage, 
       1.0f,
       FName("GrabLoop"), 
       true
    );

    if (GrabMontageTask)
    {
       GrabMontageTask->OnInterrupted.AddDynamic(this, &USFGA_Dragon_Bite::OnMontageInterrupted);
       GrabMontageTask->OnCancelled.AddDynamic(this, &USFGA_Dragon_Bite::OnMontageCancelled);
       GrabMontageTask->ReadyForActivation();
       
       GetWorld()->GetTimerManager().SetTimer(
          GrabDurationTimerHandle,
          this,
          &USFGA_Dragon_Bite::OnGrabMontageCompleted,
          GrabDuration,
          false
       );
    }
}

void USFGA_Dragon_Bite::OnDamageRecieved(UAbilitySystemComponent* Source, const FGameplayEffectSpec& SpecApplied,
    FActiveGameplayEffectHandle ActiveHandle)
{
   if (!GrabbedTarget.IsValid()) return;

   
   if (SpecApplied.GetPeriod() > 0.0f)
   {
      return; 
   }

   float CurrentTime = GetWorld()->GetTimeSeconds();
   if (CurrentTime - LastDamageTime < DamageCountCoolDown) return;

    for (const FGameplayModifierInfo& Modifier : SpecApplied.Def->Modifiers)
    {
       if (Modifier.Attribute.AttributeName == "Damage")
       {
          LastDamageTime = CurrentTime;
          CurrentHitCount++;

          if (CurrentHitCount >= RescueCount)
          {
             if (GrabDurationTimerHandle.IsValid())
             {
                GetWorld()->GetTimerManager().ClearTimer(GrabDurationTimerHandle);
             }
             
             ApplyStaggerToSelf();
             
             ASFCharacterBase* Dragon = GetSFCharacterFromActorInfo();
             if (Dragon && Dragon->GetMesh())
             {
                UAnimInstance* AnimInst = Dragon->GetMesh()->GetAnimInstance();
                if (AnimInst && BiteMontage)
                {
                   AnimInst->Montage_Stop(0.2f, BiteMontage);
                }
             }

             EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
          }
          break;
       }
    }
}

void USFGA_Dragon_Bite::ApplyExecutionDamage(AActor* Target)
{
    if (!Target || !DamageGameplayEffectClass)
       return;

    UAbilitySystemComponent* TargetASC =
       UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
    if (!TargetASC)
       return;

    UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
    if (!SourceASC)
       return;

    FGameplayEffectSpecHandle SpecHandle =
       MakeOutgoingGameplayEffectSpec(DamageGameplayEffectClass, GetAbilityLevel());

    if (!SpecHandle.IsValid())
       return;

    SpecHandle.Data->SetSetByCallerMagnitude(
       SFGameplayTags::Data_Damage_BaseDamage,
       ExecutionDamage
    );

    SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

void USFGA_Dragon_Bite::ApplyStaggerToSelf()
{
    if (!StaggerGameplayEffectClass)
       return;

    UAbilitySystemComponent* DragonASC = GetAbilitySystemComponentFromActorInfo();
    if (!DragonASC)
       return;

    FGameplayEffectSpecHandle StaggerSpec = MakeOutgoingGameplayEffectSpec(StaggerGameplayEffectClass, 1);

    if (StaggerSpec.IsValid())
    {
       StaggerSpec.Data->SetSetByCallerMagnitude(
          SFGameplayTags::Data_Stagger_BaseStagger,
          StaggerDamageOnRescue
       );

       DragonASC->ApplyGameplayEffectSpecToSelf(*StaggerSpec.Data.Get());
    }
}

AActor* USFGA_Dragon_Bite::FindPrimaryTarget()
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

void USFGA_Dragon_Bite::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(RotationTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(GrabDurationTimerHandle);
    }

    if (GrabbedTarget.IsValid())
    {
       SendReleaseEvent(GrabbedTarget.Get());
       DetachTarget(GrabbedTarget.Get());
       GrabbedTarget.Reset();
    }

    if (OnDamageRecivedHandle.IsValid())
    {
       if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
       {
          ASC->OnGameplayEffectAppliedDelegateToSelf.Remove(OnDamageRecivedHandle);
       }
       OnDamageRecivedHandle.Reset();
    }
   
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

float USFGA_Dragon_Bite::CalcScoreModifier(const FEnemyAbilitySelectContext& Context) const
{
    float Modifier = 0.f;

    const FBossEnemyAbilitySelectContext* BossContext =
       static_cast<const FBossEnemyAbilitySelectContext*>(&Context);

    
    if (BossContext && BossContext->Zone == EBossAttackZone::Melee)
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

    // 이미 Forward Pressure 중이면 감소
    if (TargetASC->HasMatchingGameplayTag(SFGameplayTags::Dragon_Pressure_Forward))
    {
       Modifier -= 300.f;
    }

    return Modifier;
}