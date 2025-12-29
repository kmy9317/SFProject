#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SFTurnInPlaceComponent.generated.h"


class ASFBaseAIController;

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class SF_API USFTurnInPlaceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFTurnInPlaceComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	UFUNCTION(BlueprintCallable, Category="Turn In Place")
	void OnTurnFinished();

	UFUNCTION(BlueprintCallable, Category="Turn In Place")
	bool IsTurning() const { return bIsTurning; }

protected:
	virtual void BeginPlay() override;

private:
	void TryTurnInPlace();
	void ExecuteTurn(float DeltaYaw);
	void EnableNaturalRotation(bool bEnable);

	ASFBaseAIController* GetAIController() const;
	APawn* GetControlledPawn() const;

private:
	UPROPERTY(EditAnywhere, Category="Turn In Place")
	float TurnThreshold = 70.f;

	UPROPERTY(EditAnywhere, Category="Turn In Place")
	float LargeTurnThreshold = 130.f;

	UPROPERTY(EditAnywhere, Category="Turn In Place")
	float CooldownSeconds = 2.f;

	float CooldownRemaining = 0.f;
	bool bIsTurning = false;
	float LockedDeltaYaw = 0.f;
};
