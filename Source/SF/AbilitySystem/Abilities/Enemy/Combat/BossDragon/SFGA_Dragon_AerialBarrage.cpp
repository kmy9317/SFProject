#include "SFGA_Dragon_AerialBarrage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/Enemy/Component/SFDragonMovementComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h" // 필수
#include "Kismet/KismetMathLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Actors/SFDragonFireballProjectile.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"
#include "Character/SFCharacterBase.h"
#include "Components/CapsuleComponent.h"

USFGA_Dragon_AerialBarrage::USFGA_Dragon_AerialBarrage()
{
    AbilityTags.AddTag(SFGameplayTags::Ability_Dragon_FlameBreath_Spin);
    ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_UsingAbility);
    ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_Invulnerable);
    ActivationOwnedTags.AddTag(SFGameplayTags::Dragon_Movement_Flying);
}

void USFGA_Dragon_AerialBarrage::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayAbilityActivationInfo ActivationInfo, 
    const FGameplayEventData* TriggerEventData)
{
   if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) 
    { 
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true); 
        return; 
    }

    // 타겟 리스트 수집
    PlayerList.Empty();
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PC = It->Get())
        {
            if (ASFCharacterBase* PlayerChar = Cast<ASFCharacterBase>(PC->GetPawn()))
            {
                PlayerList.Add(PlayerChar);
            }
        }
    }
    SelectRandomLivingTarget();

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Character) return;

    USFDragonMovementComponent* MoveComp = Cast<USFDragonMovementComponent>(Character->GetCharacterMovement());
    if (!MoveComp) return;
    
   
    MoveComp->bOrientRotationToMovement = false; 
    Character->bUseControllerRotationYaw = false;
    
 
    CachedBrakingDeceleration = MoveComp->BrakingDecelerationFlying;

    if (!MoveComp->IsFlying())
    {
        MoveComp->SetFlyingMode(true);
    }

    StartAscend();
}

void USFGA_Dragon_AerialBarrage::SelectRandomLivingTarget()
{
    TArray<ASFCharacterBase*> LivingPlayers;
    for (ASFCharacterBase* P : PlayerList)
    {
        if (P && !P->GetAbilitySystemComponent()->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
        {
            LivingPlayers.Add(P);
        }
    }
    TargetActor = (LivingPlayers.Num() > 0) ? LivingPlayers[FMath::RandRange(0, LivingPlayers.Num() - 1)] : nullptr;
}

void USFGA_Dragon_AerialBarrage::StartAscend()
{
 
    if (!GetOwningActorFromActorInfo()->HasAuthority()) return;

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Character) return;
    
  
    if (TakeOffMontage)
    {
        Character->PlayAnimMontage(TakeOffMontage, 1.0f);
    }
    
    float CurrentZ = Character->GetActorLocation().Z;

    // 이미 목표 고도라면 바로 공격
    if (CurrentZ >= TargetAltitude)
    {
        StartOrbitAttack();
        return;
    }
    
 
    float DistanceNeeded = TargetAltitude - CurrentZ;
    float CalcDuration = (AscendSpeed > 0.f) ? (DistanceNeeded / AscendSpeed) : 5.0f;
    CalcDuration += 2.0f; // 여유 시간 추가

    //  루트모션으로 강제 상승
    AscendTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
        this, 
        FName("DragonAscend"), 
        FVector::UpVector, 
        AscendSpeed, 
        CalcDuration, 
        false, 
        nullptr, 
        ERootMotionFinishVelocityMode::SetVelocity, 
        FVector::ZeroVector, 
        0.0f, 
        false
    );

    if (AscendTask) AscendTask->ReadyForActivation();
        
    TWeakObjectPtr<ACharacter> WeakChar = Character;
    
    // 고도 체크 타이머
    GetWorld()->GetTimerManager().SetTimer(
        AscendCheckTimerHandle, [this, WeakChar]()
        {
            if (WeakChar.IsValid() && WeakChar->GetActorLocation().Z >= TargetAltitude)
            {
                GetWorld()->GetTimerManager().ClearTimer(AscendCheckTimerHandle);
                
                if (AscendTask)
                {
                    AscendTask->EndTask();
                    AscendTask = nullptr;
                }
                
                WeakChar->GetCharacterMovement()->Velocity = FVector::ZeroVector;
                
                if (TakeOffMontage)
                {
                    WeakChar->StopAnimMontage(TakeOffMontage);
                }

                StartOrbitAttack();
            }
        }, 0.05f, true);
}

void USFGA_Dragon_AerialBarrage::StartOrbitAttack()
{
    
    if (!GetOwningActorFromActorInfo()->HasAuthority()) return;

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    USFDragonMovementComponent* MoveComp = Character ? Cast<USFDragonMovementComponent>(Character->GetCharacterMovement()) : nullptr;

    if (MoveComp)
    {
        MoveComp->BrakingDecelerationFlying = 0.0f;
    }


    FVector MyLoc = Character->GetActorLocation();
    FVector OrbitCenter = MapCenterLocation;
    OrbitCenter.Z = MyLoc.Z;

    FVector DirToCenter = (OrbitCenter - MyLoc).GetSafeNormal();
    FVector MyForward = Character->GetActorForwardVector();

    
    FVector TangentCCW = FVector::CrossProduct(DirToCenter, FVector::UpVector);
    
    float DotResult = FVector::DotProduct(MyForward, TangentCCW);
    bIsClockwise = (DotResult < 0.0f); 


    if (FlyingAttackMontage && MaxFireballCount > 0 && ShotsPerMontageLoop > 0)
    {
        float MontageDuration = FlyingAttackMontage->GetPlayLength();
        float TotalLoops = (float)MaxFireballCount / (float)ShotsPerMontageLoop;
        float TotalDurationNeeded = TotalLoops * MontageDuration;
        float Circumference = 2.0f * PI * OrbitRadius;

        CalculatedOrbitSpeed = (TotalDurationNeeded > 0.0f) ? (Circumference / TotalDurationNeeded) : 1500.0f;
    }

    CurrentFireballCount = 0;
    
    if (FlyingAttackMontage)
    {
        UAbilityTask_PlayMontageAndWait* MontageTask = 
            UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
                this, NAME_None, FlyingAttackMontage);
            
        if (MontageTask)
        {
            MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
            MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageEnded);
            MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageEnded);
            MontageTask->ReadyForActivation();
        }
    }
    
    UAbilityTask_WaitGameplayEvent* WaitEventTask = 
        UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
            this, SFGameplayTags::GameplayEvent_Dragon_Fireball_Launch);
        
    if (WaitEventTask)
    {
        WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnFireballEventReceived);
        WaitEventTask->ReadyForActivation();
    }
    
    GetWorld()->GetTimerManager().SetTimer(
        OrbitTimerHandle, this, &ThisClass::TickOrbitMovement, 0.02f, true);
}

void USFGA_Dragon_AerialBarrage::TickOrbitMovement()
{
    
    if (!GetOwningActorFromActorInfo()->HasAuthority()) return;

    ACharacter* OwnerChar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!OwnerChar) return;

    FVector MyLoc = OwnerChar->GetActorLocation();
    FVector OrbitCenter = MapCenterLocation;
    OrbitCenter.Z = MyLoc.Z;

    FVector DirectionToCenter = (OrbitCenter - MyLoc).GetSafeNormal();
    float DistanceFromCenter = FVector::Dist2D(MyLoc, OrbitCenter);

    //  결정된 bIsClockwise에 따라 접선 벡터 계산
    FVector TangentDir;
    if (bIsClockwise)
    {
        // 시계 방향
        TangentDir = FVector::CrossProduct(FVector::UpVector, DirectionToCenter);
    }
    else
    {
        // 반시계 방향
        TangentDir = FVector::CrossProduct(DirectionToCenter, FVector::UpVector);
    }

 
    FVector FinalDir = TangentDir;
    if (DistanceFromCenter > OrbitRadius + 200.f) 
        FinalDir = (TangentDir + DirectionToCenter * 0.4f).GetSafeNormal();
    else if (DistanceFromCenter < OrbitRadius - 200.f) 
        FinalDir = (TangentDir - DirectionToCenter * 0.4f).GetSafeNormal();

    // 속도 적용
    FVector CurrentVelocity = FinalDir * CalculatedOrbitSpeed;
    OwnerChar->GetCharacterMovement()->Velocity = CurrentVelocity;
    
   
    if (!CurrentVelocity.IsNearlyZero())
    {
        FRotator TravelRot = CurrentVelocity.Rotation();
        FRotator TargetRot = FRotator(0.f, TravelRot.Yaw, 0.f);
        OwnerChar->SetActorRotation(FMath::RInterpTo(
            OwnerChar->GetActorRotation(), TargetRot, 0.02f, 5.0f));
    }
}

void USFGA_Dragon_AerialBarrage::OnFireballEventReceived(FGameplayEventData Payload)
{
    if (CurrentFireballCount >= MaxFireballCount) return;
    
    SelectRandomLivingTarget();
    
    if (!TargetActor.IsValid()) return; 

    ACharacter* OwnerChar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!OwnerChar || !ProjectileClass) return;


    FVector SpawnLoc = OwnerChar->GetMesh()->GetSocketLocation(MuzzleSocketName);
    
    FVector TargetPos = TargetActor->GetActorLocation();
    
    ACharacter* TargetChar = Cast<ACharacter>(TargetActor.Get());
    if (TargetChar)
    {
        TargetPos.Z += TargetChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.5f;
    }
    
    FRotator LookRot = UKismetMathLibrary::FindLookAtRotation(SpawnLoc, TargetPos);
    
    FTransform SpawnTM(LookRot, SpawnLoc);
    
    ASFDragonFireballProjectile* Projectile = GetWorld()->SpawnActorDeferred<ASFDragonFireballProjectile>(
        ProjectileClass, 
        SpawnTM, 
        OwnerChar, 
        OwnerChar, 
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn
    );

    if (Projectile)
    {
        Projectile->SetOwner(GetAvatarActorFromActorInfo());
        Projectile->FinishSpawning(SpawnTM);
    }

    CurrentFireballCount++;

    if (CurrentFireballCount >= MaxFireballCount)
    {
        TryChainToDive();
    }
}

void USFGA_Dragon_AerialBarrage::OnMontageEnded()
{
  
    if (CurrentFireballCount < MaxFireballCount)
    {
        if (FlyingAttackMontage)
        {
            UAbilityTask_PlayMontageAndWait* MontageTask = 
                UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
                    this, NAME_None, FlyingAttackMontage);
            
            if (MontageTask)
            {
                MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
                MontageTask->ReadyForActivation();
            }
        }
    }
}

void USFGA_Dragon_AerialBarrage::TryChainToDive()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(OrbitTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(AscendCheckTimerHandle);
    }
    if (AscendTask) AscendTask->EndTask();

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    USFDragonMovementComponent* MoveComp = Character ? Cast<USFDragonMovementComponent>(Character->GetCharacterMovement()) : nullptr;

    if (MoveComp)
    {
        MoveComp->Velocity = FVector::ZeroVector;
        // 주의: 마찰력 복구는 EndAbility에서 수행
    }
    
    FGameplayTagContainer DiveTag;
    DiveTag.AddTag(SFGameplayTags::Ability_Dragon_DiveAttack);

    if (GetAbilitySystemComponentFromActorInfo()->TryActivateAbilitiesByTag(DiveTag))
    {
        UAbilityTask_WaitGameplayTagRemoved* WaitTask =
            UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
                this, SFGameplayTags::Ability_Dragon_DiveAttack);

        if (WaitTask)
        {
            WaitTask->Removed.AddDynamic(this, &ThisClass::OnChainAbilityEnded);
            WaitTask->ReadyForActivation();
        }
    }
    else
    {
        
        FGameplayEventData EventData;
        if (GetAbilitySystemComponentFromActorInfo()->HandleGameplayEvent(SFGameplayTags::GameplayEvent_Dragon_Flight_Land, &EventData))
        {
            UAbilityTask_WaitGameplayTagRemoved* LandWaitTask =
                UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
                    this, SFGameplayTags::Ability_Dragon_Movement_Land);

            if (LandWaitTask)
            {
                LandWaitTask->Removed.AddDynamic(this, &ThisClass::OnChainAbilityEnded);
                LandWaitTask->ReadyForActivation();
            }
        }
        else
        {
            
            EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        }
    }
}

void USFGA_Dragon_AerialBarrage::OnChainAbilityEnded()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

float USFGA_Dragon_AerialBarrage::CalcAIScore(const FEnemyAbilitySelectContext& Context) const
{
    return 999999.f; 
}

void USFGA_Dragon_AerialBarrage::EndAbility(
    const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayAbilityActivationInfo ActivationInfo, 
    bool bReplicateEndAbility, 
    bool bWasCancelled)
{
    
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(OrbitTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(AscendCheckTimerHandle);
    }

    
    if (AscendTask)
    {
        AscendTask->EndTask();
        AscendTask = nullptr;
    }

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    USFDragonMovementComponent* MoveComp = Character ? 
        Cast<USFDragonMovementComponent>(Character->GetCharacterMovement()) : nullptr;

    if (MoveComp)
    {
        MoveComp->BrakingDecelerationFlying = CachedBrakingDeceleration;

        if (bWasCancelled)
        {
            MoveComp->Velocity = FVector::ZeroVector;
            MoveComp->SetFlyingMode(false);
            MoveComp->bOrientRotationToMovement = true;
            Character->bUseControllerRotationYaw = false;
        }
        else
        {
            
            MoveComp->bOrientRotationToMovement = true;
            Character->bUseControllerRotationYaw = false;
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}