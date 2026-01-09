#pragma once

class UBoxComponent;
class UProjectileMovementComponent;

#include "CoreMinimal.h"
#include "Interaction/SFWorldPickupable.h"
#include "SFPickupableItemBase.generated.h"

UCLASS()
class SF_API ASFPickupableItemBase : public ASFWorldPickupable
{
	GENERATED_BODY()

public:
	ASFPickupableItemBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void OnRep_PickupInfo() override;
	virtual void GetMeshComponents(TArray<UMeshComponent*>& OutMeshComponents) const override;
	virtual ESFOutlineStencil GetOutlineStencil() const;

protected:
	UPROPERTY(EditDefaultsOnly)
	bool bAutoCollisionResize = true;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBoxComponent> MovementCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBoxComponent> PickupCollision;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

protected:
	UPROPERTY(EditDefaultsOnly)
	FVector2D MaxMovementCollisionExtent = FVector2D(16.f, 16.f);

	UPROPERTY(EditDefaultsOnly)
	FVector2D MinPickupCollisionExtent = FVector2D(32.f, 32.f);
};
