#include "SFPickupableItemBase.h"

#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Item/SFItemData.h"
#include "Item/SFItemDefinition.h"
#include "Item/SFItemInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "System/SFAssetManager.h"


ASFPickupableItemBase::ASFPickupableItemBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	
	bReplicates = true;
	bAlwaysRelevant = true;
	AActor::SetReplicateMovement(true);

	MovementCollision = CreateDefaultSubobject<UBoxComponent>("BoxCollision");
	MovementCollision->SetCollisionProfileName("BlockOnlyWorldObject");
	SetRootComponent(MovementCollision);

	PickupCollision = CreateDefaultSubobject<UBoxComponent>("PickupCollision");
	PickupCollision->SetCollisionProfileName("Pickupable");
	PickupCollision->SetupAttachment(GetRootComponent());
	
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("MeshComponent");
	MeshComponent->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	MeshComponent->SetCollisionProfileName("NoCollision");
	MeshComponent->SetupAttachment(GetRootComponent());

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.f;
	ProjectileMovement->Friction = 1.f;
	ProjectileMovement->BounceVelocityStopSimulatingThreshold = 0.f;
	ProjectileMovement->Velocity = FVector::ZeroVector;
}

void ASFPickupableItemBase::OnRep_PickupInfo()
{
	Super::OnRep_PickupInfo();

	TSoftObjectPtr<UStaticMesh> PickupableMeshPath = nullptr;
		
	if (const USFItemInstance* ItemInstance = PickupInfo.PickupInstance.ItemInstance)
	{
		if (const USFItemDefinition* ItemDefinition = USFItemData::Get().FindDefinitionById(ItemInstance->GetItemID()))
		{
			PickupableMeshPath = ItemDefinition->WorldMesh;
		}
	}
	else if (TSubclassOf<USFItemDefinition> ItemDefinitionClass = PickupInfo.PickupDefinition.ItemDefinitionClass)
	{
		const USFItemDefinition* ItemDefinition = ItemDefinitionClass->GetDefaultObject<USFItemDefinition>();
		PickupableMeshPath = ItemDefinition->WorldMesh;
	}

	if (PickupableMeshPath.IsNull() == false)
	{
		if (UStaticMesh* PickupableMesh = USFAssetManager::GetAssetByPath(PickupableMeshPath))
		{
			MeshComponent->SetStaticMesh(PickupableMesh);

			if (bAutoCollisionResize)
			{
				float Radius;
				FVector Origin, BoxExtent;
				UKismetSystemLibrary::GetComponentBounds(MeshComponent, Origin, BoxExtent, Radius);

				FVector MovementCollisionExtent = FVector(FMath::Min(MaxMovementCollisionExtent.X, BoxExtent.X), FMath::Min(MaxMovementCollisionExtent.Y, BoxExtent.Y), BoxExtent.Z);
				MovementCollision->SetBoxExtent(MovementCollisionExtent);
				
				FVector PickupCollisionExtent = FVector(FMath::Max(MinPickupCollisionExtent.X, BoxExtent.X), FMath::Max(MinPickupCollisionExtent.Y, BoxExtent.Y), BoxExtent.Z);
				PickupCollision->SetBoxExtent(PickupCollisionExtent);
			}
		}
	}
}

void ASFPickupableItemBase::GetMeshComponents(TArray<UMeshComponent*>& OutMeshComponents) const
{
	OutMeshComponents.Add(MeshComponent);
}
