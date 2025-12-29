#pragma once

#include "CoreMinimal.h"
#include "Interaction/SFWorldInteractable.h"
#include "SFChestBase.generated.h"

class UArrowComponent;

UENUM(BlueprintType)
enum class ESFChestState : uint8
{
	Open,
	Close
};

UCLASS()
class SF_API ASFChestBase : public ASFWorldInteractable
{
	GENERATED_BODY()

public:
	ASFChestBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ~ Begin ISFInteractable
	virtual FSFInteractionInfo GetPreInteractionInfo(const FSFInteractionQuery& InteractionQuery) const override;
	virtual void GetMeshComponents(TArray<UMeshComponent*>& OutMeshComponents) const override;
	// ~ End ISFInteractable

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SetChestState(ESFChestState NewChestState);

private:
	UFUNCTION()
	void OnRep_ChestState();

protected:

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing=OnRep_ChestState)
	ESFChestState ChestState = ESFChestState::Close;

	UPROPERTY(EditAnywhere, Category="SF|Info")
	FSFInteractionInfo OpenedInteractionInfo;
	
	UPROPERTY(EditAnywhere, Category="SF|Info")
	FSFInteractionInfo ClosedInteractionInfo;
	
	UPROPERTY(EditDefaultsOnly, Category="SF|Info")
	TObjectPtr<UAnimMontage> OpenMontage;

	UPROPERTY(EditDefaultsOnly, Category="SF|Info")
	TObjectPtr<UAnimMontage> CloseMontage;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UArrowComponent> ArrowComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> MeshComponent;
};
