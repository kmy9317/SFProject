// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Animation/AnimInstance.h"
#include "SFEnemyAnimInstance.generated.h"

class ASFCharacterBase;
class UCharacterMovementComponent;
class UAbilitySystemComponent;

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
    Hold,        // 유지: 현재 값 그대로
    BlendOut     // 감쇠: 0으로 서서히 줄이기
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

    // Turn In Place
    void UpdateTurnInPlace(float DeltaSeconds);
    ;
    // RootYawOffset 모드별 처리
    void ProcessAccumulateMode(float DeltaYaw);
    void ProcessHoldMode();
    void ProcessBlendOutMode(float DeltaSeconds);

    // 각도 정규화 (-180 ~ 180)
    float NormalizeAxis(float Angle);

    // Spring 시뮬레이션 함수
    float SpringInterpolate(float Current, float Target, float DeltaTime, float& Velocity,
                           float Stiffness, float DampingRatio, float Mass);

public:
    // 애니메이션 그래프에서 RemainingTurnYaw 커브 값을 받아서 처리
    UFUNCTION(BlueprintCallable, Category = "Turn In Place")
    void ProcessRemainingTurnYaw(float RemainingTurnYaw);

    // Turn In Place 애니메이션이 끝났을 때 호출 
    UFUNCTION(BlueprintCallable, Category = "Turn In Place")
    void OnTurnInPlaceCompleted();



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

    FVector PreviousWorldVelocity2D;
    // Turn Cache
    FRotator PreviousRotation;  // 이전 프레임의 회전값

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

    //Turn In Place Data
    // RootYawOffset 모드
    UPROPERTY(BlueprintReadWrite, Category = "Turn In Place")
    ERootYawOffsetMode RootYawOffsetMode = ERootYawOffsetMode::Accumulate;

    // 루트 본 Yaw 오프셋 (스켈레탈 메시 반대 회전용)
    UPROPERTY(BlueprintReadWrite, Category = "Turn In Place")
    float RootYawOffset = 0.0f;

    // Turn In Place 트리거 임계값
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
    float TurnInPlaceThreshold = 90.0f;

    // 180도 Turn 트리거 임계값
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
    float TurnInPlaceThreshold_180 = 135.0f;

    // BlendOut 속도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place")
    float BlendOutSpeed = 5.0f;

    // Turn In Place 중인지
    UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
    bool bIsTurningInPlace = false;

    // 회전 방향 (Left: -1, Right: 1)
    UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
    float TurnDirection = 0.0f;

    // Turn 각도 (90.0 or 180.0)
    UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
    float TurnAngle = 90.0f;

    // 그래프 관련 변수들
    UPROPERTY(BlueprintReadWrite, Category = "Turn In Place")
    float TurnYawCurveValue = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Turn In Place")
    float PreviousTurnYawCurveValue = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Turn In Place")
    float PreviousRemainingTurnYaw;

    UPROPERTY(BlueprintReadWrite, Category = "Turn In Place")
    float YawDeltaSinceLastUpdate = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Turn In Place")
    bool bEnableRootYawOffset = true;
    UPROPERTY(BlueprintReadWrite, Category = "Turn In Place")
    float SmoothedRootYawOffset;
    // Spring 시뮬레이션 파라미터
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place|Spring")
    float SpringStiffness = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place|Spring")
    float SpringDampingRatio = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn In Place|Spring")
    float SpringMass = 1.0f;

    // Spring 시뮬레이션 상태
    float SpringVelocity = 0.0f;
    float SpringCurrentValue = 0.0f;
};