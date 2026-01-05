// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "AI/SFAIGameplayTags.h"
#include "Animation/AnimInstance.h"
#include "SFEnemyAnimInstance.generated.h"

class ASFCharacterBase;
class UCharacterMovementComponent;
class UAbilitySystemComponent;
class ASFBaseAIController;
enum class EAIRotationMode : uint8;

UENUM(BlueprintType)
enum class AE_CardinalDirection : uint8
{
    Forward    UMETA(DisplayName = "Forward"),
    Right      UMETA(DisplayName = "Right"),
    Backward   UMETA(DisplayName = "Backward"),
    Left       UMETA(DisplayName = "Left")
};


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

    void UpdateAimOffsetData(float DeltaSeconds);
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

    UFUNCTION(BlueprintCallable, Category = "Thread Safe Function", meta = (BlueprintThreadSafe))
    bool ShouldDistanceMatchStop() const;

    UFUNCTION(BlueprintCallable, Category = "Thread Safe Function", meta = (BlueprintThreadSafe))
    float GetPredictedStopDistance() const;

    //각도를 4분면으로 나뉘어서 Deadzone과 이전 방향 유지를 할껀지를 확인하고 방향을 반환
    UFUNCTION(BlueprintCallable, Category = "Thread Safe Function", meta = (BlueprintThreadSafe))
    AE_CardinalDirection GetCardinalDirectionFromAngle(float Angle, float DeadZone, AE_CardinalDirection CurrentDirection, bool bUseCurrentDirection) const;



public:

    UFUNCTION(BlueprintPure, Category = "Turn In Place")
    bool IsTurningInPlace() const { return bIsTurningInPlace; }



protected:

    UPROPERTY(EditDefaultsOnly, Category = "GameplayTags")
    FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;


    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character")
    TObjectPtr<ACharacter> Character;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character")
    TObjectPtr<UCharacterMovementComponent> CachedMovementComponent;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character")
    TObjectPtr<ASFBaseAIController> CachedAIController;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character")
    TObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;

private:

    FVector CachedLocation;
    FVector PreviousWorldLocation;
    bool bIsFirstUpdate;

    FRotator CachedRotation;

    FVector CachedWorldVelocity;
    FVector CachedWorldVelocity2D;

    FVector CachedWorldAcceleration2D;

    FVector PreviousWorldVelocity2D;
    FRotator PreviousRotation;

    float CachedDeltaSeconds;
    float CachedControlRotationYaw;
    EAIRotationMode CachedRotationMode;

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Character")
    bool bUsingAbility = false;
    
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

    //에임 오프셋을 위한 변수
    UPROPERTY(BlueprintReadOnly, Category = "SF|Animation|AimOffset")
    float AimPitch;

    UPROPERTY(BlueprintReadOnly, Category = "SF|Animation|AimOffset")
    float AimYaw;

    // ControlRotation 전체를 캐싱해야 Pitch
    UPROPERTY(Transient)
    FRotator CachedControlRotation; 

    
    
    //  Velocity Data 
    UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
    bool bHasVelocity;

    UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
    FVector WorldVelocity2D;

    UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
    FVector LocalVelocity2D;
    
    UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
    float GroundSpeed;
    
    UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
    float FlySpeed;
    
    UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
    float LocalVelocityDirectionAngle;

    UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
    AE_CardinalDirection LocalVelocityDirection;

    UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
    bool bWasMovingLastFrame;

    //방향 전환의 마지노선 일단 10으로 둔다 
    UPROPERTY(BlueprintReadOnly, Category = "Cardinal Direction")
    float CardinalDirectionDeadZone;

    //  Acceleration Data
    UPROPERTY(BlueprintReadOnly, Category = "Acceleration Data")
    bool bHasAcceleration;

    UPROPERTY(BlueprintReadOnly, Category = "Acceleration Data")
    FVector WorldAcceleration2D;

    UPROPERTY(BlueprintReadOnly, Category = "Acceleration Data")
    FVector LocalAcceleration2D;
    
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="State")
    bool bIsTurningInPlace = false;

};