#include "SFEquipmentBase.h"

#include "Character/SFCharacterBase.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"

ASFEquipmentBase::ASFEquipmentBase(const FObjectInitializer& ObjectInitializer)
	: Super( ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	bReplicates = true;

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>("ArrowComponent");
	ArrowComponent->PrimaryComponentTick.bStartWithTickEnabled = false;
	SetRootComponent(ArrowComponent);
	
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("WeaponMesh");
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetupAttachment(GetRootComponent());
	MeshComponent->PrimaryComponentTick.bStartWithTickEnabled = false;
	MeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	
	TraceDebugCollision = CreateDefaultSubobject<UBoxComponent>("TraceDebugCollision");
	TraceDebugCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TraceDebugCollision->SetGenerateOverlapEvents(false);
	TraceDebugCollision->SetupAttachment(GetRootComponent());
	TraceDebugCollision->PrimaryComponentTick.bStartWithTickEnabled = false;
}

UAbilitySystemComponent* ASFEquipmentBase::GetAbilitySystemComponent() const
{
	if (ASFCharacterBase* SFCharacter = Cast<ASFCharacterBase>(GetOwner()))
	{
		return SFCharacter->GetAbilitySystemComponent();
	}
	return nullptr;
}

void ASFEquipmentBase::BeginPlay()
{
	Super::BeginPlay();
	
}

