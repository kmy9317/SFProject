// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ControllerComponent.h"
#include "SFCombatComponentBase.generated.h"

class USFAbilitySystemComponent;
struct FEnemyAbilitySelectContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatStateChanged, bool, bInCombat);


UCLASS(Abstract, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFCombatComponentBase : public UControllerComponent
{
    GENERATED_BODY()

public:
    
    USFCombatComponentBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
    
    virtual void InitializeCombatComponent();
    
    UFUNCTION(BlueprintPure, Category = "SF|Combat")
    AActor* GetCurrentTarget() const { return CurrentTarget; }

    
    virtual void UpdateTargetActor(AActor* NewTarget);

    
    virtual bool SelectAbility(
        const FEnemyAbilitySelectContext& Context,
        const FGameplayTagContainer& SearchTags,
        FGameplayTag& OutSelectedTag);

    
    UPROPERTY(BlueprintAssignable, Category = "Combat")
    FOnCombatStateChanged OnCombatStateChanged;

protected:
    
    virtual void EvaluateTarget() PURE_VIRTUAL(USFCombatComponentBase::EvaluateTarget, );

    
    void SetGameplayTagStatus(const FGameplayTag& Tag, bool bActive);

    
    APawn* GetOwnerPawn() const;

    
    UPROPERTY()
    TObjectPtr<USFAbilitySystemComponent> CachedASC;

    
    UPROPERTY()
    TObjectPtr<AActor> CurrentTarget;

    
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float ScoreDifferenceThreshold = 100.f;
};

