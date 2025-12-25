// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "SFDragonCombatComponent.generated.h"


UENUM(BlueprintType)
enum class EBossAttackZone : uint8
{
    None        UMETA(DisplayName = "None"),
    Melee       UMETA(DisplayName = "Melee Range"),
    Mid         UMETA(DisplayName = "Mid Range"),
    Long        UMETA(DisplayName = "Long Range"),
    OutOfRange  UMETA(DisplayName = "Out of Range")
};

UCLASS()
class SF_API USFDragonCombatComponent : public USFEnemyCombatComponent
{
    GENERATED_BODY()

public:
    USFDragonCombatComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void InitializeCombatComponent() override;

    // Threat System
    UFUNCTION()
    void AddThreat(float ThreatValue, AActor* Actor);

    AActor* GetHighestThreatActor();

    void UpdateTargetFromThreat();
    
    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    EBossAttackZone GetTargetLocationZone() const;

    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    float GetDistanceToTarget() const;

    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    float GetAngleToTarget() const;
    
    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    float GetPlayerHealthPercent() const { return PlayerHealthPercent; }

    virtual bool SelectAbility(const FEnemyAbilitySelectContext& Context, const FGameplayTagContainer& SearchTags, FGameplayTag& OutSelectedTag) override;
    
protected:
    // Update Functions
    void UpdateSpatialData();
    void MonitorTargetState();

    void StartSpatialUpdateTimer();
    void StopSpatialUpdateTimer();

    void StartStateMonitorTimer();
    void StopStateMonitorTimer();

    void StartThreatUpdateTimer();
    void StopThreatUpdateTimer();

    bool IsValidTarget(AActor* Target) const;

protected:
    // Threat System
    UPROPERTY()
    TMap<AActor*, float> ThreatMap;

    // Cached Spatial Data
    UPROPERTY()
    EBossAttackZone CurrentZone;

    UPROPERTY()
    float CachedDistance;

    UPROPERTY()
    float CachedAngle;

    // Zone Ranges
    UPROPERTY(EditAnywhere, Category = "AI|Setup")
    float MeleeRange = 1500.f;

    UPROPERTY(EditAnywhere, Category = "AI|Setup")
    float MidRange = 2500.f;

    UPROPERTY(EditAnywhere, Category = "AI|Setup")
    float LongRange = 3500.f;



    UPROPERTY()
    float PlayerHealthPercent;
    

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

    // 마지막으로 선택한 어빌리티 태그 (연속 공격 방지용)
    FGameplayTag LastSelectedAbilityTag;
};