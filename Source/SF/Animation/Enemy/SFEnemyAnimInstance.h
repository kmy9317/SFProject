// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
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

UENUM(BlueprintType)
enum class ERootYawOffsetMode : uint8
{
    Accumulate,  // 누적: RootYawOffset에 회전값 계속 쌓기
    Hold,        // 유지: 현재 값 그대로 (Turn 애니메이션 재생 중)
    BlendOut     // 감쇠: 0으로 서서히 줄이기 (Turn 완료 후)
};


UCLASS(Abstract)
class SF_API USFEnemyAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    USFEnemyAnimInstance(const FObjectInitializer& ObjectInitializer);

    virtual void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC);

    //Helper
public:
    UFUNCTION(BlueprintCallable, Category = "Animation|TurnInPlace")
    bool RequestTurnInPlace(float TargetYaw, bool bForceImmediate = true);

protected:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    void ApplySpringToRootYawOffset(float DeltaSeconds);
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

    // Turn In Place (RootMotion 전용)
    void UpdateTurnInPlace(float DeltaSeconds);

    // RootYawOffset 모드별 처리
    void ProcessAccumulateMode(float DeltaYaw);
    void ProcessHoldMode();
    void ProcessBlendOutMode(float DeltaSeconds);

    // RemainingTurnYaw 커브 처리
    void ProcessRemainingTurnYaw(float DeltaTurnYaw);

    // Turn 완료 처리
    void OnTurnInPlaceAnimationComplete();



public:
    // Turn In Place 애니메이션이 끝났을 때 호출 (AnimNotify에서 호출 가능)
    UFUNCTION(BlueprintCallable, Category = "Turn In Place")
    void OnTurnInPlaceCompleted();

    // TurnInPlace 중인지 확인 (BT Task에서 사용)
    UFUNCTION(BlueprintPure, Category = "Turn In Place")
    bool IsTurningInPlace() const { return bIsTurningInPlace; }

    // 현재 RootYawOffset 값 반환 (BT Task에서 남은 회전량 판단용)
    UFUNCTION(BlueprintPure, Category = "Turn In Place")
    float GetRootYawOffset() const { return RootYawOffset; }

    // 남은 회전량 절대값 반환 (BT Task에서 완료 판단용)
    UFUNCTION(BlueprintPure, Category = "Turn In Place")
    float GetRemainingTurnYaw() const { return FMath::Abs(RootYawOffset); }

    // TurnInPlace 상태 완전 리셋 (RotationMode 전환 시 호출)
    UFUNCTION(BlueprintCallable, Category = "Turn In Place")
    void ResetTurnInPlaceState();



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

    //  Velocity Data 
    UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
    bool bHasVelocity;

    UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
    FVector WorldVelocity2D;

    UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
    FVector LocalVelocity2D;
    
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


    // ========== Turn In Place Data (RootMotion 전용) ==========

    // RootYawOffset 모드
    UPROPERTY(BlueprintReadWrite, Category = "Turn In Place")
    ERootYawOffsetMode RootYawOffsetMode = ERootYawOffsetMode::Accumulate;

    // RootYawOffset: 애니메이션 각도와 실제 필요 각도의 차이를 보정
    // 예: 실제 85도 필요, 애니메이션 90도 → RootYawOffset = -5도
    UPROPERTY(BlueprintReadWrite, Category = "Turn In Place")
    float RootYawOffset = 0.0f;

    // 90도 Turn 트리거 임계값
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
    float TurnInPlaceThreshold = 65.0f;

    // 180도 Turn 트리거 임계값
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
    float TurnInPlaceThreshold_180 = 130.0f;

    // BlendOut 속도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
    float BlendOutSpeed = 5.0f;

    // Turn In Place 중인지
    UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
    bool bIsTurningInPlace = false;

    // 회전 방향 (Left: -1, Right: 1)
    UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
    float TurnDirection = 0.0f;

    // Turn 각도 (90.0 or 180.0) - 애니메이션이 회전하는 각도
    UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
    float TurnAngle = 90.0f;

    // 실제 필요한 회전 각도 (예: 85도, 95도 등)
    UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
    float ActualTurnYaw = 0.0f;

    // RemainingTurnYaw 커브 추적용
    UPROPERTY(BlueprintReadWrite, Category = "Turn In Place")
    float PreviousRemainingTurnYaw = 90.0f;

    bool bIsForcedTurn;
};