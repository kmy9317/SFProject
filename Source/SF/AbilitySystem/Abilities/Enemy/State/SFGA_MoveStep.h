#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_MoveStep.generated.h"


UCLASS()
class SF_API USFGA_MoveStep : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_MoveStep();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:

	UPROPERTY(EditAnywhere, Category = "Movement")
	TObjectPtr<UAnimMontage> ForwardAnim; 


	UPROPERTY(EditAnywhere, Category = "Movement")
	TObjectPtr<UAnimMontage> BackwardAnim;
	
	UPROPERTY(EditAnywhere, Category = "Movement")
	float StepIntensity = 1200.f;
	
	UPROPERTY(EditAnywhere, Category = "Movement")
	float UpForce = 0.1f;

private:
	UFUNCTION()
	void OnMoveStepFinished();
};