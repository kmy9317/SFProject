// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Animation/AnimInstance.h"
#include "SFEnemyAnimInstance.generated.h"

class ASFCharacterBase;
class UCharacterMovementComponent;
class UAbilitySystemComponent;

/**
 * Thread-Safe Optimized Animation Instance for Enemy Characters
 */
UCLASS(Abstract)
class SF_API USFEnemyAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    USFEnemyAnimInstance(const FObjectInitializer& ObjectInitializer);

    virtual void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC);

protected:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category = "Animation")
    UCharacterMovementComponent* GetMovementComponent();
    
    
    // ========== Thread Safe Update Functions ==========
    UFUNCTION(BlueprintCallable, Category = "Thread Safe Function", meta = (BlueprintThreadSafe))
    void UpdateLocationData(float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category = "Thread Safe Function", meta = (BlueprintThreadSafe))
    void UpdateRotationData();

    UFUNCTION(BlueprintCallable, Category = "Thread Safe Function", meta = (BlueprintThreadSafe))
    void UpdateVelocityData();

    UFUNCTION(BlueprintCallable, Category = "Thread Safe Function", meta = (BlueprintThreadSafe))
    void UpdateAccelerationData();

    UFUNCTION(BlueprintCallable, Category = "DistanceMatch Function", meta = (BlueprintThreadSafe))
    bool ShouldDistanceMatchStop() const;

    UFUNCTION(BlueprintCallable, Category = "DistanceMatch Function", meta = (BlueprintThreadSafe))
    float GetPredictedStopDistance() const;

    // ========== Debug Functions ==========
    UFUNCTION(BlueprintCallable, Category = "Animation Debug")
    void PrintAnimationDebugInfo() const;

protected:

    UPROPERTY(EditDefaultsOnly, Category = "GameplayTags")
    FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;


    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character")
    TObjectPtr<ACharacter> Character;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character")
    TObjectPtr<UCharacterMovementComponent> CachedMovementComponent;

private:
    
    // Location Cache
    FVector CachedLocation;
    FVector PreviousWorldLocation;
    bool bIsFirstUpdate;
    
    // Rotation Cache
    FRotator CachedRotation;
    
    // Velocity Cache
    FVector CachedWorldVelocity;
    FVector CachedWorldVelocity2D;
    
    // Acceleration Cache
    FVector CachedWorldAcceleration2D;

protected:
    
    //  Location Data
    UPROPERTY(BlueprintReadOnly, Category = "Location Data")
    FVector WorldLocation;

    UPROPERTY(BlueprintReadOnly, Category = "Location Data")
    float DisplacementSinceLastUpdate;

    UPROPERTY(BlueprintReadOnly, Category = "Location Data")
    float DisplacementSpeed;

    //Rotation Data 
    UPROPERTY(BlueprintReadOnly, Category = "Rotation Data")
    FRotator WorldRotation;

    //  Velocity Data 
    UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
    bool bHasVelocity;

    UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
    FVector WorldVelocity2D;

    UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
    FVector LocalVelocity2D;

    //  Acceleration Data 
    UPROPERTY(BlueprintReadOnly, Category = "Acceleration Data")
    bool bHasAcceleration;

    UPROPERTY(BlueprintReadOnly, Category = "Acceleration Data")
    FVector WorldAcceleration2D;

    UPROPERTY(BlueprintReadOnly, Category = "Acceleration Data")
    FVector LocalAcceleration2D;
};