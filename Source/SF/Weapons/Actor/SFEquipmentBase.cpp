#include "SFEquipmentBase.h"

#include "Character/SFCharacterBase.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"

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
	MeshComponent->SetCollisionProfileName("Weapon");
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetupAttachment(GetRootComponent());
	MeshComponent->PrimaryComponentTick.bStartWithTickEnabled = false;
	MeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	
	TraceDebugCollision = CreateDefaultSubobject<UBoxComponent>("TraceDebugCollision");
	TraceDebugCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TraceDebugCollision->SetGenerateOverlapEvents(false);
	TraceDebugCollision->SetupAttachment(GetRootComponent());
	TraceDebugCollision->PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ASFEquipmentBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bIsBlocking);
}

UAbilitySystemComponent* ASFEquipmentBase::GetAbilitySystemComponent() const
{
	if (ASFCharacterBase* SFCharacter = Cast<ASFCharacterBase>(GetOwner()))
	{
		return SFCharacter->GetAbilitySystemComponent();
	}
	return nullptr;
}

void ASFEquipmentBase::SetIsBlocking(bool bNewBlockingState)
{
	if (HasAuthority())
	{
		bIsBlocking = bNewBlockingState;
		OnRep_IsBlocking();
	}
}

void ASFEquipmentBase::OnRep_IsBlocking()
{
	
}

