#pragma once

#include "CoreMinimal.h"
#include "AI/Controller/SFBaseAIController.h"
#include "SFDragonController.generated.h"

class USFTurnInPlaceComponent;

UCLASS()
class SF_API ASFDragonController : public ASFBaseAIController
{
	GENERATED_BODY()

public:
	ASFDragonController();

	virtual void InitializeAIController() override;
    
	//~ Begin ASFBaseAIController Interface
	virtual bool IsTurningInPlace() const override;
	virtual float GetTurnThreshold() const override;
	virtual bool ShouldRotateActorByController() const override;
	//~ End ASFBaseAIController Interface

	UFUNCTION(BlueprintCallable, Category="AI|TurnInPlace")
	USFTurnInPlaceComponent* GetTurnInPlaceComponent() const { return TurnInPlaceComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|TurnInPlace")
	TObjectPtr<USFTurnInPlaceComponent> TurnInPlaceComponent;
};