#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"
#include "Interface/ISFDragonPressureInterface.h"
#include "SFGA_Dragon_Bite.generated.h"

/**
 * Dragon Bite Attack Ability
 * Monster Hunter style Grab & Rescue mechanic
 */
UCLASS()
class SF_API USFGA_Dragon_Bite : public USFGA_Enemy_BaseAttack, public ISFDragonPressureInterface
{
    GENERATED_BODY()

public:
    USFGA_Dragon_Bite();

    virtual EDragonPressureType GetPressureType() const override { return EDragonPressureType::Forward; }
    virtual float GetPressureDuration() const override { return PressureDuration; }
    virtual TSubclassOf<UGameplayEffect> GetPressureEffectClass() const override { return PressureEffectClass; }

    virtual float CalcScoreModifier(const FEnemyAbilitySelectContext& Context) const override;

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
    void StartBiteAttack();
    void PlayBiteLoop();
    void PlayGrabMontage();

    UFUNCTION()
    void OnBiteMontageCompleted();

    UFUNCTION()
    void OnGrabMontageCompleted();

    UFUNCTION()
    void OnMontageInterrupted();

    UFUNCTION()
    void OnMontageCancelled();

    UFUNCTION()
    void OnBiteHit(FGameplayEventData Payload);

    void TriggerGrabAbilityOnTarget(AActor* Target);
    void SendReleaseEvent(AActor* Target);

    void AttachTargetToJaw(AActor* Target);
    void DetachTarget(AActor* Target);

    UFUNCTION()
    void OnDamageRecieved(UAbilitySystemComponent* Source, const FGameplayEffectSpec& SpecApplied,FActiveGameplayEffectHandle ActiveHandle);

    void ApplyExecutionDamage(AActor* Target);
    void ApplyStaggerToSelf();


    void UpdateRotationToTarget();
    AActor* FindPrimaryTarget();

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Dragon|Montage")
    TObjectPtr<UAnimMontage> BiteMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Dragon|Effect")
    TSubclassOf<UGameplayEffect> GrabGameplayEffectClass;

    UPROPERTY(EditDefaultsOnly, Category = "Dragon|Effect")
    TSubclassOf<UGameplayEffect> StaggerGameplayEffectClass;

    UPROPERTY(EditDefaultsOnly, Category = "Dragon|Bite")
    int32 BiteCount = 2;
    
    UPROPERTY(EditDefaultsOnly, Category = "Dragon|Bite")
    float RotationSpeed = 10.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Dragon|Grab")
    FName JawSocketName = "JawSocket";

    UPROPERTY(EditDefaultsOnly, Category = "Dragon|Grab")
    float GrabDuration = 5.f;

    UPROPERTY(EditDefaultsOnly, Category = "Dragon|Grab")
    float ExecutionDamage = 9999.f;

    UPROPERTY(EditDefaultsOnly, Category = "Dragon|Rescue")
    int32 RescueCount = 3;

    UPROPERTY(EditDefaultsOnly, Category = "Dragon|Rescue")
    float DamageCountCoolDown = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Dragon|Rescue")
    float StaggerDamageOnRescue = 50.f;

    UPROPERTY(EditDefaultsOnly, Category = "Dragon|Pressure")
    float PressureDuration = 5.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Dragon|Pressure")
    TSubclassOf<UGameplayEffect> PressureEffectClass;

private:
    UPROPERTY()
    TWeakObjectPtr<AActor> GrabbedTarget;
    
    UPROPERTY()
    TWeakObjectPtr<AActor> PrimaryTarget;

    int32 CurrentBiteCount = 0;
    float LastDamageTime = -999.f;
    int32 CurrentHitCount = 0;

    FTimerHandle GrabDurationTimerHandle;
    FTimerHandle RotationTimerHandle;

    FDelegateHandle OnDamageRecivedHandle;
};