#include "SFMeleeWeaponActor.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFMeleeWeaponActor)

ASFMeleeWeaponActor::ASFMeleeWeaponActor(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    // Static Mesh를 Root로
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    RootComponent = StaticMeshComponent;
    StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StaticMeshComponent->SetSimulatePhysics(false);

    // Collision Box
    WeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollision"));
    WeaponCollision->SetupAttachment(StaticMeshComponent);
    WeaponCollision->SetCollisionObjectType(ECC_WorldDynamic);
    WeaponCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    WeaponCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    WeaponCollision->SetGenerateOverlapEvents(true);
    WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponCollision->SetBoxExtent(FVector(50.f, 10.f, 50.f));
    
    WeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &ASFMeleeWeaponActor::OnWeaponOverlap);
}

bool ASFMeleeWeaponActor::CanBeTraced() const
{
    return CurrentWeaponOwner != nullptr;
}

void ASFMeleeWeaponActor::OnTraceStart(AActor* WeaponOwner)
{
    if (!WeaponOwner) return;

    CurrentWeaponOwner = WeaponOwner;
    HitActorsThisAttack.Empty();

    if (WeaponCollision)
    {
        WeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }
    
}

void ASFMeleeWeaponActor::OnTraceEnd(AActor* WeaponOwner)
{
    if (WeaponCollision)
    {
        WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    CurrentWeaponOwner = nullptr;
    HitActorsThisAttack.Empty();
}

void ASFMeleeWeaponActor::OnWeaponOverlap( UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 유효성 검사
    if (!OtherActor || !CurrentWeaponOwner) return;
    if (OtherActor == this || OtherActor == CurrentWeaponOwner) return;
    if (HitActorsThisAttack.Contains(OtherActor)) return;

    // 히트 기록
    HitActorsThisAttack.Add(OtherActor);

    // HitResult 생성
    FHitResult HitInfo;
    HitInfo.HitObjectHandle = FActorInstanceHandle(OtherActor);
    HitInfo.Component = OtherComp;

    // 맞은 부위 판단
    if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(OtherComp))
    {
        FVector WeaponLocation = WeaponCollision->GetComponentLocation();
        HitInfo.BoneName = SkeletalMesh->FindClosestBone(WeaponLocation);
        HitInfo.ImpactPoint = SkeletalMesh->GetBoneLocation(HitInfo.BoneName);
    }
    else
    {
        HitInfo.BoneName = NAME_None;
        HitInfo.ImpactPoint = OtherActor->GetActorLocation();
    }

    HitInfo.Location = HitInfo.ImpactPoint;
    HitInfo.ImpactNormal = FVector::UpVector;
    HitInfo.bBlockingHit = false;

    // 데미지 처리
    OnTraced(HitInfo, CurrentWeaponOwner);
}

void ASFMeleeWeaponActor::OnTraced(const FHitResult& HitInfo, AActor* WeaponOwner)
{
    if (!WeaponOwner) return;

    UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(WeaponOwner);
    if (!OwnerASC) return;

    // EventData 생성
    FGameplayEventData EventData;
    EventData.Instigator = WeaponOwner;
    EventData.Target = HitInfo.GetActor();
    EventData.OptionalObject = this;
    EventData.ContextHandle = OwnerASC->MakeEffectContext();

    // TargetData 생성
    FGameplayAbilityTargetDataHandle TargetDataHandle;
    FGameplayAbilityTargetData_SingleTargetHit* TargetData = 
        new FGameplayAbilityTargetData_SingleTargetHit(HitInfo);
    TargetDataHandle.Add(TargetData);
    EventData.TargetData = TargetDataHandle;

    // GameplayEvent 전송
    OwnerASC->HandleGameplayEvent(SFGameplayTags::GameplayEvent_TraceHit, &EventData);
}