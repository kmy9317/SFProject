#include "SFAutoPickup.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/SFPlayerState.h"
#include "Item/Fragments/SFItemFragment_AutoPickup.h"

ASFAutoPickup::ASFAutoPickup()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    AActor::SetReplicateMovement(true);

    // 루트 컴포넌트 (바닥만 충돌)
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionSphere->InitSphereRadius(16.f);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    RootComponent = CollisionSphere;

    // 수집 범위
    CollectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollectionSphere"));
    CollectionSphere->InitSphereRadius(50.f);
    CollectionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    CollectionSphere->SetGenerateOverlapEvents(false);
    CollectionSphere->SetupAttachment(RootComponent);

    // 자석 감지 범위
    DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
    DetectionSphere->InitSphereRadius(500.f);
    DetectionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    DetectionSphere->SetGenerateOverlapEvents(false);
    DetectionSphere->SetupAttachment(RootComponent);

    // 투사체 이동 (낙하용)
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->bShouldBounce = true;
    ProjectileMovement->Bounciness = 0.3f;
    ProjectileMovement->Friction = 0.8f;
    ProjectileMovement->BounceVelocityStopSimulatingThreshold = 20.f;
    ProjectileMovement->ProjectileGravityScale = 1.f;
    ProjectileMovement->bIsHomingProjectile = false;
}

void ASFAutoPickup::Initialize(USFItemInstance* InItemInstance, int32 InAmount)
{
    ItemInstance = InItemInstance;
    Amount = FMath::Max(1, InAmount);

    if (ItemInstance && ItemInstance->GetOuter() != this)
    {
        ItemInstance->Rename(nullptr, this);
    }

    if (const USFItemFragment_AutoPickup* Fragment = ItemInstance->FindFragmentByClass<USFItemFragment_AutoPickup>())
    {
        bApplyToAllPlayers = Fragment->bApplyToAllPlayers;
        DetectionRadius = Fragment->DetectionRadius;
        CollectionRadius = Fragment->CollectionRadius;
        HomingAcceleration = Fragment->HomingAcceleration;
        MaxHomingSpeed = Fragment->HomingSpeed;
        ActivationDelay = Fragment->InitialDelay;
    }

    // 랜덤 방향으로 튀어오르기
    float RandomYaw = FMath::FRandRange(0.f, 360.f);
    FRotator LaunchRotation(-(90.f - DropInitialAngle), RandomYaw, 0.f);
    FVector LaunchDirection = LaunchRotation.Vector();

    ProjectileMovement->Velocity = LaunchDirection * DropInitialSpeed;
}

void ASFAutoPickup::BeginPlay()
{
    Super::BeginPlay();

    DetectionSphere->SetSphereRadius(DetectionRadius);
    CollectionSphere->SetSphereRadius(CollectionRadius);

    if (HasAuthority())
    {
        DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASFAutoPickup::OnDetectionBeginOverlap);
        CollectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASFAutoPickup::OnCollectionBeginOverlap);

        // 타이머로 감지 활성화
        GetWorldTimerManager().SetTimer(
            ActivationTimerHandle,
            this,
            &ASFAutoPickup::EnableOverlapDetection,
            ActivationDelay,
            false
        );
    }
}

void ASFAutoPickup::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!HasAuthority() || PickupState != ESFAutoPickupState::Homing)
    {
        return;
    }

    AActor* Target = HomingTarget.Get();
    if (!Target)
    {
        PickupState = ESFAutoPickupState::Idle;
        DetectionSphere->SetGenerateOverlapEvents(true);
        ProjectileMovement->Velocity = FVector::ZeroVector;
        return;
    }

    // 가속
    CurrentHomingSpeed = FMath::Min(CurrentHomingSpeed + HomingAcceleration * DeltaTime, MaxHomingSpeed);

    // 타겟 방향으로 Velocity 설정
    FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    ProjectileMovement->Velocity = Direction * CurrentHomingSpeed;
}

void ASFAutoPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ThisClass, PickupState);
}

void ASFAutoPickup::EnableOverlapDetection()
{
    PickupState = ESFAutoPickupState::Idle;

    DetectionSphere->SetGenerateOverlapEvents(true);
    CollectionSphere->SetGenerateOverlapEvents(true);

    // 이미 오버랩 중인 액터 체크
    TArray<AActor*> OverlappingActors;
    CollectionSphere->GetOverlappingActors(OverlappingActors, ACharacter::StaticClass());

    for (AActor* Actor : OverlappingActors)
    {
        if (IsPlayerCharacter(Actor))
        {
            Collect(Actor);
            return;
        }
    }

    DetectionSphere->GetOverlappingActors(OverlappingActors, ACharacter::StaticClass());

    for (AActor* Actor : OverlappingActors)
    {
        if (IsPlayerCharacter(Actor))
        {
            StartHoming(Actor);
            return;
        }
    }
}

void ASFAutoPickup::StartHoming(AActor* Target)
{
    if (!Target)
    {
        return;
    }

    PickupState = ESFAutoPickupState::Homing;
    HomingTarget = Target;
    CurrentHomingSpeed = 0.f;
    DetectionSphere->SetGenerateOverlapEvents(false);

    // ProjectileMovement 재활성화
    ProjectileMovement->SetUpdatedComponent(CollisionSphere);
    ProjectileMovement->SetActive(true, true);
    ProjectileMovement->ProjectileGravityScale = 0.f;

    // 충돌 비활성화
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASFAutoPickup::OnDetectionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority() || PickupState != ESFAutoPickupState::Idle)
    {
        return;
    }

    if (!IsPlayerCharacter(OtherActor))
    {
        return;
    }

    StartHoming(OtherActor);
}

void ASFAutoPickup::OnCollectionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority() || PickupState == ESFAutoPickupState::Collected || PickupState == ESFAutoPickupState::Waiting)
    {
        return;
    }

    if (!IsPlayerCharacter(OtherActor))
    {
        return;
    }

    Collect(OtherActor);
}

void ASFAutoPickup::Collect(AActor* Collector)
{
    PickupState = ESFAutoPickupState::Collected;

    GetWorldTimerManager().ClearTimer(ActivationTimerHandle);

    ProjectileMovement->StopMovementImmediately();
    ProjectileMovement->SetActive(false);

    if (bApplyToAllPlayers)
    {
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            if (APlayerController* PC = It->Get())
            {
                if (ASFPlayerState* PS = PC->GetPlayerState<ASFPlayerState>())
                {
                    ApplyCollectEffect(PS, Amount);
                }
            }
        }
    }
    else
    {
        if (APawn* Pawn = Cast<APawn>(Collector))
        {
            if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
            {
                if (ASFPlayerState* PS = PC->GetPlayerState<ASFPlayerState>())
                {
                    ApplyCollectEffect(PS, Amount);
                }
            }
        }
    }

    SetLifeSpan(0.1f);
}

bool ASFAutoPickup::IsPlayerCharacter(AActor* Actor) const
{
    ACharacter* Character = Cast<ACharacter>(Actor);
    if (!Character)
    {
        return false;
    }

    AController* Controller = Character->GetController();
    return Controller && Controller->IsPlayerController();
}

void ASFAutoPickup::OnRep_PickupState()
{
    // BP에서 VFX/SFX 처리
}
