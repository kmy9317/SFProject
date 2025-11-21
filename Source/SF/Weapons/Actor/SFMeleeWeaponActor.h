// SFMeleeWeaponActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/SFTraceActorInterface.h"
#include "SFMeleeWeaponActor.generated.h"

UCLASS()
class SF_API ASFMeleeWeaponActor : public AActor, public ISFTraceActorInterface
{
	GENERATED_BODY()

public:
	ASFMeleeWeaponActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Tick(float DeltaTime) override;

	void PerformTrace();

	// ISFTraceActorInterface
	virtual bool CanBeTraced() const override;
	virtual void OnTraced(const FHitResult& HitInfo, AActor* WeaponOwner) override;
	virtual void OnTraceStart(AActor* WeaponOwner) override;
	virtual void OnTraceEnd(AActor* WeaponOwner) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Mesh)
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(EditDefaultsOnly, Category= "Weapon|Trace")
	bool bShowTrace = false;
    
	UPROPERTY(EditDefaultsOnly, Category= "Weapon|Trace")
	float TraceRadius = 50.f;

	UPROPERTY(EditDefaultsOnly, Category= "Weapon|Trace")
	TArray<FName> TraceSockets;

private:
	
	TObjectPtr<AActor> CurrentWeaponOwner;
	
	TMap<FName, FVector> PreviousSocketLocations;
	
	TSet<AActor*> HitActorsThisAttack;
};