// SFEnemyController.h
#pragma once

#include "CoreMinimal.h"
#include "SFBaseAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "SFEnemyController.generated.h"

class USFEnemyCombatComponent;

UCLASS()
class SF_API ASFEnemyController : public ASFBaseAIController
{
	GENERATED_BODY()

public:
	ASFEnemyController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitializeAIController() override;
	
	UFUNCTION(BlueprintCallable, Category = "AI|Combat")
	void SetTargetForce(AActor* NewTarget);

protected:
	//~ Begin ASFBaseAIController Interface
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;
	virtual bool ShouldRotateActorByController() const override;
	virtual float GetTurnThreshold() const override;
	virtual bool IsTurningInPlace() const override;
	//~ End ASFBaseAIController Interface


	void RotateActorTowardsController(float DeltaTime);
	
	UFUNCTION()
	void OnCombatStateChanged(bool bInCombat);

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	// Sight Configuration
	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	float SightRadius = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	float LoseSightRadius = 2500.f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	float PeripheralVisionAngleDegrees = 90.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "AI|Target")
	FName TargetTag = NAME_None;
};