#pragma once

#include "CoreMinimal.h"
#include "SFInteractable.h"
#include "SFPickupable.h"
#include "GameFramework/Actor.h"
#include "SFWorldPickupable.generated.h"

UCLASS()
class SF_API ASFWorldPickupable : public AActor, public ISFInteractable, public ISFPickupable
{
	GENERATED_BODY()

public:
	ASFWorldPickupable(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

public:
	virtual FSFInteractionInfo GetPreInteractionInfo(const FSFInteractionQuery& InteractionQuery) const override { return InteractionInfo; }
	virtual void SetPickupInfo(const FSFPickupInfo& InPickupInfo);
	virtual FSFPickupInfo GetPickupInfo() const override { return PickupInfo; }

protected:
	UFUNCTION()
	virtual void OnRep_PickupInfo();

protected:
	UPROPERTY(EditAnywhere, Category="Info")
	FSFInteractionInfo InteractionInfo;
	
	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_PickupInfo, Category="Info")
	FSFPickupInfo PickupInfo;
};
