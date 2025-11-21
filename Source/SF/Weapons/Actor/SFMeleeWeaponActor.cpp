
#include "SFMeleeWeaponActor.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFMeleeWeaponActor)


ASFMeleeWeaponActor::ASFMeleeWeaponActor(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    RootComponent = StaticMeshComponent;

    StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StaticMeshComponent->SetSimulatePhysics(false);
}

void ASFMeleeWeaponActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
  
    if (CurrentWeaponOwner)
    {
        PerformTrace();
    }
}

bool ASFMeleeWeaponActor::CanBeTraced() const
{
    return CurrentWeaponOwner != nullptr;
}

void ASFMeleeWeaponActor::OnTraceStart(AActor* WeaponOwner)
{
    CurrentWeaponOwner = WeaponOwner;
    HitActorsThisAttack.Empty();
    PreviousSocketLocations.Empty();  

    // Tick 활성화
    SetActorTickEnabled(true);

    // 초기 소켓 위치 저장
    if (StaticMeshComponent)
    {
        for (const FName& Socket : TraceSockets)
        {
            if (StaticMeshComponent->DoesSocketExist(Socket))
            {
                FVector Location = StaticMeshComponent->GetSocketLocation(Socket);
                PreviousSocketLocations.Emplace(Socket, Location);  
            }
        }
    }
}

void ASFMeleeWeaponActor::OnTraceEnd(AActor* WeaponOwner)
{
  
    SetActorTickEnabled(false);

    CurrentWeaponOwner = nullptr;
    PreviousSocketLocations.Empty();
    HitActorsThisAttack.Empty();
}

void ASFMeleeWeaponActor::PerformTrace()
{
    if (!CurrentWeaponOwner || !StaticMeshComponent || TraceSockets.Num() < 2)
        return;

    UWorld* World = GetWorld();
    if (!World)
        return;

   
    for (int32 i = 0; i < TraceSockets.Num() - 1; ++i)
    {
        FName StartSocket = TraceSockets[i];
        FName EndSocket = TraceSockets[i + 1];

        // 현재 프레임 위치
        FVector CurrentStart = StaticMeshComponent->GetSocketLocation(StartSocket);
        FVector CurrentEnd = StaticMeshComponent->GetSocketLocation(EndSocket);

        // 이전 프레임 위치
        FVector* PrevStartPtr = PreviousSocketLocations.Find(StartSocket);
        FVector* PrevEndPtr = PreviousSocketLocations.Find(EndSocket);

        // 첫 프레임 처리
        if (!PrevStartPtr || !PrevEndPtr)
        {
            // 첫 프레임에도 현재 위치에서 작은 범위 Trace 수행
            PreviousSocketLocations.Emplace(StartSocket, CurrentStart);
            PreviousSocketLocations.Emplace(EndSocket, CurrentEnd);

            // 첫 프레임: 현재 위치에서 짧은 거리 Trace
            TArray<FHitResult> InitialHitResults;
            FCollisionQueryParams InitialQueryParams;
            InitialQueryParams.AddIgnoredActor(this);
            InitialQueryParams.AddIgnoredActor(CurrentWeaponOwner);
            InitialQueryParams.bTraceComplex = false;

            // 현재 위치에서 약간 앞으로 Trace 
            FVector TraceDirection = (CurrentEnd - CurrentStart).GetSafeNormal();
            FVector ShortTraceEnd = CurrentEnd + TraceDirection * TraceRadius;

            World->SweepMultiByChannel(
                InitialHitResults,
                CurrentEnd,
                ShortTraceEnd,
                FQuat::Identity,
                ECC_Pawn,
                FCollisionShape::MakeSphere(TraceRadius),
                InitialQueryParams
            );

            // 첫 프레임 Hit 처리
            for (const FHitResult& Hit : InitialHitResults)
            {
                if (AActor* HitActor = Hit.GetActor())
                {
                    if (!HitActorsThisAttack.Contains(HitActor))
                    {
                        HitActorsThisAttack.Add(HitActor);
                        OnTraced(Hit, CurrentWeaponOwner);
                    }
                }
            }

            continue;
        }

        // Collision Query 설정
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);
        QueryParams.AddIgnoredActor(CurrentWeaponOwner);
        QueryParams.bTraceComplex = false;

        TArray<FHitResult> HitResults;

        // 이전 프레임과 현재 프레임의 중심점 계산
        FVector PrevCenter = (*PrevStartPtr + *PrevEndPtr) / 2.0f;
        FVector CurrentCenter = (CurrentStart + CurrentEnd) / 2.0f;

        // 캡슐의 Half Height (StartSocket과 EndSocket 사이 거리의 절반)
        float CapsuleHalfHeight = FVector::Dist(CurrentStart, CurrentEnd) / 2.0f;

        // 캡슐의 방향 계산 (StartSocket에서 EndSocket 방향)
        FVector CapsuleDirection = (CurrentEnd - CurrentStart).GetSafeNormal();
        FQuat CapsuleRotation = FQuat::FindBetweenNormals(FVector::UpVector, CapsuleDirection);

        // Capsule Sweep Trace 
        World->SweepMultiByChannel(
            HitResults,
            PrevCenter,
            CurrentCenter,
            CapsuleRotation,
            ECC_Pawn,
            FCollisionShape::MakeCapsule(TraceRadius, CapsuleHalfHeight),
            QueryParams
        );

        // Hit 처리
        for (const FHitResult& Hit : HitResults)
        {
            if (AActor* HitActor = Hit.GetActor())
            {
                if (!HitActorsThisAttack.Contains(HitActor))
                {
                    HitActorsThisAttack.Add(HitActor);

                    OnTraced(Hit, CurrentWeaponOwner);
                }
            }
        }
        
        if (bShowTrace)
        {
            // 캡슐 시각화
            DrawDebugCapsule(
                World,
                CurrentCenter,
                CapsuleHalfHeight,
                TraceRadius,
                CapsuleRotation,
                FColor::Red,
                false,
                0.1f,
                0,
                2.0f
            );
        }

        // 위치 업데이트
        PreviousSocketLocations.Emplace(StartSocket, CurrentStart);
        PreviousSocketLocations.Emplace(EndSocket, CurrentEnd);
    }
}

void ASFMeleeWeaponActor::OnTraced(const FHitResult& HitInfo, AActor* WeaponOwner)
{
    
    UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(WeaponOwner);
    if (!OwnerASC)
    {
        return;
    }

    // EventData 생성
    FGameplayEventData EventData;
    EventData.Instigator = CurrentWeaponOwner;
    EventData.Target = HitInfo.GetActor();
    EventData.OptionalObject = this;
    EventData.ContextHandle = OwnerASC->MakeEffectContext();
    
    
    //HitResult를  TargetData
    FGameplayAbilityTargetDataHandle TargetDataHandle;
    FGameplayAbilityTargetData_SingleTargetHit* TargetData = new FGameplayAbilityTargetData_SingleTargetHit(HitInfo);
    TargetDataHandle.Add(TargetData);
    EventData.TargetData = TargetDataHandle;
    
    //GameplayEvent 전송
    OwnerASC->HandleGameplayEvent(SFGameplayTags::GameplayEvent_TraceHit, &EventData);
    
}