// SFGA_Dragon_Charge.h
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"
#include "SFGA_Dragon_Charge.generated.h"


UCLASS()
class SF_API USFGA_Dragon_Charge : public USFGA_Enemy_BaseAttack
{
	GENERATED_BODY()

public:
	USFGA_Dragon_Charge();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,bool bReplicateEndAbility,bool bWasCancelled) override;
	

	void FinishCharge(bool bCancelled);

	UFUNCTION()
	void OnMoveFinished(bool bSuccess);

	UFUNCTION()
	void OnMoveCancelled();

	UFUNCTION()
	void OnMontageEnded();
	

	UFUNCTION()
	void OnChargeOverlap( UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,bool bFromSweep, const FHitResult& SweepResult);
	
	virtual float CalcScoreModifier(const FEnemyAbilitySelectContext& Context) const override;

protected:
	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* ChargeMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Dragon|Charge Movement")
	TObjectPtr<class UCurveFloat> ChargeSpeedCurve;

	UPROPERTY(EditDefaultsOnly)
	float ChargeDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly)
	float StopDistance = 200.f;

	TEnumAsByte<ECollisionResponse> OriginalPawnResponse;

	UPROPERTY()
	TSet<TWeakObjectPtr<AActor>> HitActors;
};