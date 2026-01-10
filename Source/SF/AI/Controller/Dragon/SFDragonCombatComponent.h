// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "AI/Controller/SFCombatComponentBase.h"
#include "SFDragonCombatComponent.generated.h"

struct FSFPhaseData;

UENUM(BlueprintType)
enum class EBossAttackZone : uint8
{
    None        UMETA(DisplayName = "None"),
    Melee       UMETA(DisplayName = "Melee Range"),
    Mid         UMETA(DisplayName = "Mid Range"),
    Long        UMETA(DisplayName = "Long Range"),
    OutOfRange  UMETA(DisplayName = "Out of Range")
};


UENUM(BlueprintType)
enum class EBossTargetState : uint8
{
    None         UMETA(DisplayName = "None"),
    Locked       UMETA(DisplayName = "Locked"),      
    Grace        UMETA(DisplayName = "Grace"),       
    ForceRelease UMETA(DisplayName = "Force Release") 
};


UCLASS()
class SF_API USFDragonCombatComponent : public USFCombatComponentBase
{
    GENERATED_BODY()

public:
    USFDragonCombatComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
    
    virtual void InitializeCombatComponent() override;
    
    UFUNCTION()
    void AddThreat(float ThreatValue, AActor* Actor);

    AActor* GetHighestThreatActor();
    void CleanupThreatMap();


    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    EBossAttackZone GetTargetLocationZone() const { return CurrentZone; }

    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    float GetDistanceToTarget() const { return CachedDistance; }

    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    float GetAngleToTarget() const { return CachedAngle; }

    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    float GetPlayerHealthPercent() const { return PlayerHealthPercent; }

   
    virtual bool SelectAbility(const FEnemyAbilitySelectContext& Context, const FGameplayTagContainer& SearchTags, FGameplayTag& OutSelectedTag) override;

protected:

    virtual void EvaluateTarget() override;
    
    void UpdateSpatialData();

 
    void MonitorTargetState();


    void StartSpatialUpdateTimer();
    void StopSpatialUpdateTimer();
    void StartStateMonitorTimer();
    void StopStateMonitorTimer();
    void StartThreatUpdateTimer();
    void StopThreatUpdateTimer();
    
    bool IsValidTarget(AActor* Target) const;
    bool ShouldForceReleaseTarget(AActor* Target) const;

    void CheckPhaseTransitions();

    void OnHealthChanged(const FOnAttributeChangeData& OnAttributeChangeData);

protected:
    // Target State Machine
    UPROPERTY()
    EBossTargetState CurrentTargetState = EBossTargetState::None;

    UPROPERTY()
    float LastValidTargetTime = 0.f;

    UPROPERTY(EditAnywhere, Category = "AI|Target Hold")
    float TargetGraceDuration = 0.5f;

    UPROPERTY(EditAnywhere, Category = "AI|Target Hold")
    float MaxCombatRange = 5000.f;


    UPROPERTY()
    TMap<AActor*, float> ThreatMap;

    // Cached Spatial Data
    UPROPERTY()
    EBossAttackZone CurrentZone = EBossAttackZone::None;

    UPROPERTY()
    float CachedDistance = 0.f;

    UPROPERTY()
    float CachedAngle = 0.f;

    // Zone Ranges
    UPROPERTY(EditAnywhere, Category = "AI|Zone")
    float MeleeRange = 1500.f;

    UPROPERTY(EditAnywhere, Category = "AI|Zone")
    float MidRange = 2500.f;

    UPROPERTY(EditAnywhere, Category = "AI|Zone")
    float LongRange = 3500.f;

    // Player state
    UPROPERTY()
    float PlayerHealthPercent = 1.0f;

    // Update Intervals
    UPROPERTY(EditAnywhere, Category = "AI|Update")
    float SpatialUpdateInterval = 0.1f;

    UPROPERTY(EditAnywhere, Category = "AI|Update")
    float StateMonitorInterval = 0.2f;

    UPROPERTY(EditAnywhere, Category = "AI|Update")
    float ThreatUpdateInterval = 0.5f;

    // Timers
    FTimerHandle SpatialUpdateTimerHandle;
    FTimerHandle StateMonitorTimerHandle;
    FTimerHandle ThreatUpdateTimerHandle;

  
    FGameplayTag LastSelectedAbilityTag;

    UPROPERTY()
    TArray<FSFPhaseData> PhaseData;

    UPROPERTY()
    TArray<FSFPhaseData> TriggerPhase;

    FGameplayTag PendingPhaseTag;
    
    UPROPERTY()
    TArray<FGameplayTag> RecentAbilityHistory;
    
    UPROPERTY(EditAnywhere, Category = "AI")
    int32 MaxHistorySize = 2;
    
};