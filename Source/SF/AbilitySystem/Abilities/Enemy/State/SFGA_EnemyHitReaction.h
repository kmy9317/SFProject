#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_EnemyHitReaction.generated.h"

class UAbilityTask_PlayMontageAndWait;

UCLASS()
class SF_API USFGA_EnemyHitReaction : public USFGameplayAbility
{
    GENERATED_BODY()

public:
    USFGA_EnemyHitReaction();

protected:
    UPROPERTY(EditDefaultsOnly, Category = "SFGA|HitReaction")
    UAnimMontage* FrontHitMontage;

    UPROPERTY(EditDefaultsOnly, Category = "SFGA|HitReaction")
    UAnimMontage* BackHitMontage;

    // 피격 후 무적 시간 
    UPROPERTY(EditDefaultsOnly, Category = "SFGA|HitReaction|Invincibility")
    float InvincibilityDuration = 2.0f;

    // 피격 후 무적 활성화 여부
    UPROPERTY(EditDefaultsOnly, Category = "SFGA|HitReaction|Invincibility")
    bool bActivateInvincibilityOnEnd = true;

    // 현재 무적 상태인지
    bool bIsInvincible = false;
    
    UAbilityTask_PlayMontageAndWait* MontageTask;
    FActiveGameplayEffectHandle ActiveHitEffectHandle;
    FTimerHandle InvincibilityTimerHandle;
    
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
       const FGameplayAbilityActorInfo* ActorInfo, 
       const FGameplayAbilityActivationInfo ActivationInfo, 
       const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, 
       const FGameplayAbilityActorInfo* ActorInfo, 
       const FGameplayAbilityActivationInfo ActivationInfo, 
       bool bReplicateEndAbility, bool bWasCancelled) override;

    // 무적 활성화 
    void ActivateInvincibility();

    // 무적 해제 
    UFUNCTION()
    void DeactivateInvincibility();

    UFUNCTION()
    void OnMontageCompleted();

    UFUNCTION()
    void OnMontageCancelled();

    UFUNCTION()
    void OnMontageInterrupted();
    
    FVector ExtractHitLocationFromEvent(const FGameplayEventData* EventData) const;
    FVector ExtractHitDirectionFromEvent(const FGameplayEventData* EventData) const;
};
