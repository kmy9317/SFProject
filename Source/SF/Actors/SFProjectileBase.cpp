#include "SFProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h" // [추가] 헤더 포함
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "Character/SFCharacterBase.h"

ASFProjectileBase::ASFProjectileBase()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    CollisionComponent->InitSphereRadius(15.0f);
    CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
    SetRootComponent(CollisionComponent);


    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
    ProjectileMesh->SetupAttachment(CollisionComponent); 
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); 

   
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->UpdatedComponent = CollisionComponent;
    ProjectileMovement->InitialSpeed = InitialSpeed;
    ProjectileMovement->MaxSpeed = MaxSpeed;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->ProjectileGravityScale = GravityScale;


    ProjectileEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ProjectileEffect"));
    ProjectileEffect->SetupAttachment(RootComponent);
    ProjectileEffect->bAutoActivate = true;
    ProjectileEffect->SetIsReplicated(true);

    InitialLifeSpan = LifeSpan;
}

void ASFProjectileBase::SetOwner(AActor* NewOwner)
{
    Super::SetOwner(NewOwner);

    ASFCharacterBase* SFCharacter = Cast<ASFCharacterBase>(NewOwner);
    if (SFCharacter)
    {
       OwnerChar = SFCharacter;
       if (CollisionComponent && SFCharacter)
       {
          CollisionComponent->IgnoreActorWhenMoving(OwnerChar, true);
       }
    }
}

void ASFProjectileBase::BeginPlay()
{
    Super::BeginPlay();

    CollisionComponent->OnComponentHit.AddDynamic(this, &ASFProjectileBase::OnProjectileHit);
}

void ASFProjectileBase::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    Destroy();
}