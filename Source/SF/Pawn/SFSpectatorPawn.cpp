#include "SFSpectatorPawn.h"

#include "Camera/SFCameraComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Player/SFPlayerState.h"

ASFSpectatorPawn::ASFSpectatorPawn(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    PrimaryActorTick.TickGroup = TG_PostPhysics;

    
    SetActorEnableCollision(false);
    if (GetCollisionComponent())
    {
        GetCollisionComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
    }

    SetReplicateMovement(false);
    if (GetMovementComponent())
    {
        GetMovementComponent()->Deactivate();
    }

    CameraComponent = CreateDefaultSubobject<USFCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(RootComponent);
}

void ASFSpectatorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // 부모 호출 안 함 - 기본 WASD/마우스 입력 바인딩 방지
    //Super::SetupPlayerInputComponent(PlayerInputComponent);
}

FVector ASFSpectatorPawn::GetPawnViewLocation() const
{
    if (AActor* TargetActor = FollowTarget.Get())
    {
        // 대상이 폰(캐릭터)이라면 해당 폰 시점 오프셋을 계산
        if (APawn* TargetPawn = Cast<APawn>(TargetActor))
        {
            FVector TargetEyeLocation = TargetPawn->GetPawnViewLocation();
            FVector TargetRootLocation = TargetPawn->GetActorLocation();
            // 눈 높이 오프셋 계산 (Vector from Root to Eye)
            FVector EyeOffset = TargetEyeLocation - TargetRootLocation;
            // 내 현재 위치(스무딩된 위치)에 오프셋을 더해서 리턴
            return GetActorLocation() + EyeOffset;
        }
    }

    // 대상이 없으면 기본 동작 (내 ActorLocation + BaseEyeHeight)
    return Super::GetPawnViewLocation();
}

void ASFSpectatorPawn::BeginPlay()
{
    Super::BeginPlay();

    // 카메라 모드 델리게이트 바인딩
    if (CameraComponent)
    {
        CameraComponent->DetermineCameraModeDelegate.BindDynamic(this, &ThisClass::DetermineCameraMode);
    }
}

void ASFSpectatorPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    AActor* Target = FollowTarget.Get();
    if (!Target)
    {
        return;
    }

    // Raw 타겟 위치/회전
    const FVector RawTargetLocation = Target->GetActorLocation();
    
    FRotator RawTargetRotation = FRotator::ZeroRotator;
    if (APawn* TargetPawn = Cast<APawn>(Target))
    {
        if (ASFPlayerState* TargetPS = TargetPawn->GetPlayerState<ASFPlayerState>())
        {
            RawTargetRotation = TargetPS->GetReplicatedViewRotation();
        }
        else 
        {
            RawTargetRotation = TargetPawn->GetBaseAimRotation();
        }
    }
    
    // 타겟 위치 스무딩 (1차 보간 - 네트워크 지터 흡수)
    SmoothedTargetLocation = FMath::VInterpTo(
        SmoothedTargetLocation,
        RawTargetLocation,
        DeltaTime,
        TargetSmoothingSpeed
    );

    SmoothedTargetRotation = FMath::RInterpTo(
        SmoothedTargetRotation,
        RawTargetRotation,
        DeltaTime,
        TargetSmoothingSpeed
    );


    // SpectatorPawn 위치 업데이트 (2차 보간)
    const FVector NewLocation = FMath::VInterpTo(
        GetActorLocation(),
        SmoothedTargetLocation,
        DeltaTime,
        LocationFollowSpeed
    );
    SetActorLocation(NewLocation);

    if (AController* MyController = GetController())
    {
        const FRotator CurrentRotation = MyController->GetControlRotation();
        const FRotator NewRotation = FMath::RInterpTo(
            CurrentRotation,
            SmoothedTargetRotation,
            DeltaTime,
            RotationFollowSpeed
        );
        MyController->SetControlRotation(NewRotation);
    }
}

void ASFSpectatorPawn::SetFollowTarget(AActor* InTarget)
{
    FollowTarget = InTarget;

    if (InTarget)
    {
        const FVector TargetLocation = InTarget->GetActorLocation();
        
        SmoothedTargetLocation = TargetLocation;
        SetActorLocation(TargetLocation);

        if (APawn* TargetPawn = Cast<APawn>(InTarget))
        {
            FRotator TargetRotation = FRotator::ZeroRotator;
            
            if (ASFPlayerState* TargetPS = TargetPawn->GetPlayerState<ASFPlayerState>())
            {
                TargetRotation = TargetPS->GetReplicatedViewRotation();
            }
            else
            {
                TargetRotation = TargetPawn->GetBaseAimRotation();
            }

            SmoothedTargetRotation = TargetRotation;
            
            if (AController* MyController = GetController())
            {
                MyController->SetControlRotation(TargetRotation);
            }
        }
    }
}

TSubclassOf<USFCameraMode> ASFSpectatorPawn::DetermineCameraMode()
{
    return DefaultCameraModeClass;
}
