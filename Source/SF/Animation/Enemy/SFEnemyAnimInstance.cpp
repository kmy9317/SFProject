// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/Enemy/SFEnemyAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/SFCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AnimCharacterMovementLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFEnemyAnimInstance)

USFEnemyAnimInstance::USFEnemyAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Character(nullptr)
	, CachedMovementComponent(nullptr)
	, CachedLocation(FVector::ZeroVector)
	, PreviousWorldLocation(FVector::ZeroVector)
	, bIsFirstUpdate(true)
	, CachedRotation(FRotator::ZeroRotator)
	, CachedWorldVelocity(FVector::ZeroVector)
	, CachedWorldVelocity2D(FVector::ZeroVector)
	, CachedWorldAcceleration2D(FVector::ZeroVector)
	, WorldLocation(FVector::ZeroVector)
	, DisplacementSinceLastUpdate(0.0f)
	, DisplacementSpeed(0.0f)
	, WorldRotation(FRotator::ZeroRotator)
	, bHasVelocity(false)
	, WorldVelocity2D(FVector::ZeroVector)
	, LocalVelocity2D(FVector::ZeroVector)
	, bHasAcceleration(false)
	, WorldAcceleration2D(FVector::ZeroVector)
	, LocalAcceleration2D(FVector::ZeroVector)
	, LocalVelocityDirectionAngle(0.0f)
	, CardinalDirectionDeadZone(10.f)
	, bWasMovingLastFrame(false)
	, PreviousWorldVelocity2D(FVector::ZeroVector)
	, PreviousRotation(FRotator::ZeroRotator)
	, PreviousRemainingTurnYaw(0.0f)
	,SmoothedRootYawOffset(0.0f)
{
}

#pragma region AnimationFunction

void USFEnemyAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* ASC)
{
	check(ASC);
	GameplayTagPropertyMap.Initialize(this, ASC);
}

void USFEnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (AActor* OwningActor = GetOwningActor())
	{
		Character = Cast<ACharacter>(OwningActor);
		if (Character)
		{
			CachedMovementComponent = Character->GetCharacterMovement();
		}
        
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor))
		{
			InitializeWithAbilitySystem(ASC);
		}
	}
}

void USFEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// GameThread로 값을 읽어 와야하는것을 여기서 처리함 
	if (!Character)
	{
		Character = Cast<ACharacter>(GetOwningActor());
		if (Character && !CachedMovementComponent)
		{
			CachedMovementComponent = Character->GetCharacterMovement();
		}
	}

	if (!Character || !CachedMovementComponent)
	{
		return;
	}

	// 이전 프레임 값 저장 
	if (!bIsFirstUpdate)
	{
		PreviousWorldLocation = CachedLocation;
		PreviousWorldVelocity2D = CachedWorldVelocity2D;
		PreviousRotation = CachedRotation;
	}
	else
	{
		PreviousWorldLocation = Character->GetActorLocation();
		PreviousWorldVelocity2D = FVector::ZeroVector;
		PreviousRotation = Character->GetActorRotation();
		bIsFirstUpdate = false;
	}

	CachedLocation = Character->GetActorLocation();
	CachedRotation = Character->GetActorRotation();  

	// Velocity 캐싱
	CachedWorldVelocity = CachedMovementComponent->Velocity;
	CachedWorldVelocity2D = FVector(CachedWorldVelocity.X, CachedWorldVelocity.Y, 0.0f);

	// Acceleration 캐싱
	const FVector Acceleration = CachedMovementComponent->GetCurrentAcceleration();
	CachedWorldAcceleration2D = FVector(Acceleration.X, Acceleration.Y, 0.0f);

	// Turn In Place 커브 처리 
	if (bIsTurningInPlace)
	{
		float CurrentRemainingYaw = GetCurveValue(FName("RemainingTurnYaw"));
		float DeltaYaw = PreviousRemainingTurnYaw - CurrentRemainingYaw;
		
		if (FMath::Abs(DeltaYaw) > UE_SMALL_NUMBER)
		{
			ProcessRemainingTurnYaw(DeltaYaw);
		}
		
		PreviousRemainingTurnYaw = CurrentRemainingYaw;
	}
	else
	{
		// Turn이 시작될 때 초기화
		PreviousRemainingTurnYaw = 0.0f;
	}
}

void USFEnemyAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	UpdateLocationData(DeltaSeconds);
	UpdateRotationData();
	UpdateVelocityData();
	UpdateAccelerationData();
	
	
	UpdateTurnInPlace(DeltaSeconds);
	
	// Spring 보간 적용 
	if (bEnableRootYawOffset)
	{
		ApplySpringToRootYawOffset(DeltaSeconds);
	}
}

UCharacterMovementComponent* USFEnemyAnimInstance::GetMovementComponent()
{
	if (CachedMovementComponent)
	{
		return CachedMovementComponent;
	}

	if (AActor* OwningActor = GetOwningActor())
	{
		if (ACharacter* OwningCharacter = Cast<ACharacter>(OwningActor))
		{
			CachedMovementComponent = OwningCharacter->GetCharacterMovement();
			return CachedMovementComponent;
		}
	}
	return nullptr;
}

#pragma endregion 

#pragma region ThreadSafeAnimationFunction

void USFEnemyAnimInstance::UpdateLocationData(float DeltaSeconds)
{
	WorldLocation = CachedLocation;

	// Displacement 계산
	DisplacementSinceLastUpdate = FVector::Dist(WorldLocation, PreviousWorldLocation);

	// Displacement Speed 계산
	if (DeltaSeconds > 0.0f)
	{
		DisplacementSpeed = DisplacementSinceLastUpdate / DeltaSeconds;
	}
	else
	{
		DisplacementSpeed = 0.0f;
	}
}

void USFEnemyAnimInstance::UpdateRotationData()
{
	WorldRotation = CachedRotation;
}

void USFEnemyAnimInstance::UpdateVelocityData()
{
	WorldVelocity2D = CachedWorldVelocity2D;
	
	// Local 좌표계로 변환
	LocalVelocity2D = CachedRotation.UnrotateVector(WorldVelocity2D);
	
	// Velocity 체크
	const float VelocityLength = LocalVelocity2D.Size();
	
	if (VelocityLength > 1.0f)
	{
		bHasVelocity = true;
	}
	else
	{
		bHasVelocity = false;
	}

	// 로컬 좌표계 각도
	LocalVelocityDirectionAngle = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity2D.Y, LocalVelocity2D.X));

	LocalVelocityDirection = GetCardinalDirectionFromAngle(LocalVelocityDirectionAngle, CardinalDirectionDeadZone, LocalVelocityDirection, bWasMovingLastFrame);

	bWasMovingLastFrame = bHasVelocity;
}

void USFEnemyAnimInstance::UpdateAccelerationData()
{
	// 서버: 실제 Acceleration 사용
	// 클라이언트: Velocity 변화량으로 Acceleration 추정
	if (GetOwningActor() && GetOwningActor()->HasAuthority())
	{
		WorldAcceleration2D = CachedWorldAcceleration2D;
	}
	else
	{
		// 클라이언트는 Velocity 변화량으로 Acceleration 추정
		const UWorld* World = GetWorld();
		if (World)
		{
			const float DeltaTime = World->GetDeltaSeconds();
			if (DeltaTime > UE_SMALL_NUMBER) 
			{
				WorldAcceleration2D = (CachedWorldVelocity2D - PreviousWorldVelocity2D) / DeltaTime;
			}
			else
			{
				WorldAcceleration2D = FVector::ZeroVector;
			}
		}
		else
		{
			WorldAcceleration2D = FVector::ZeroVector;
		}
	}
    
	// Local 좌표계로 변환
	LocalAcceleration2D = CachedRotation.UnrotateVector(WorldAcceleration2D);
    
	// Acceleration 체크
	const float AccelerationLength = LocalAcceleration2D.Size();
	bHasAcceleration = AccelerationLength > 1.0f;
}

bool USFEnemyAnimInstance::ShouldDistanceMatchStop() const
{
	return bHasVelocity && (!bHasAcceleration);
}

float USFEnemyAnimInstance::GetPredictedStopDistance() const
{
	if (!CachedMovementComponent) return 0.f;

	const FVector Velocity = CachedMovementComponent->GetLastUpdateVelocity();
	const bool bUseSeparateBrakingFriction = CachedMovementComponent->bUseSeparateBrakingFriction;
	const float BrakingFriction = CachedMovementComponent->BrakingFriction;
	const float GroundFriction = CachedMovementComponent->GroundFriction;
	const float BrakingFrictionFactor = CachedMovementComponent->BrakingFrictionFactor;
	const float BrakingDecel = CachedMovementComponent->BrakingDecelerationWalking;

	const FVector PredictedStopLoc = UAnimCharacterMovementLibrary::PredictGroundMovementStopLocation(
		Velocity,
		bUseSeparateBrakingFriction,
		BrakingFriction,
		GroundFriction,
		BrakingFrictionFactor,
		BrakingDecel
	);

	return FVector2D(PredictedStopLoc).Length();
}

AE_CardinalDirection USFEnemyAnimInstance::GetCardinalDirectionFromAngle(float Angle, float DeadZone,
	AE_CardinalDirection CurrentDirection, bool bUseCurrentDirection) const
{
	float AbsAngle = FMath::Abs(Angle);
	float FwdDeadZone = DeadZone;
	float BwdDeadZone = DeadZone;

	// 현재 direction에 가중치를 주기 위한 처리
	if (bUseCurrentDirection)
	{
		switch (CurrentDirection)
		{
		case AE_CardinalDirection::Forward:
			FwdDeadZone *= 2.0f;
			break;
		case AE_CardinalDirection::Backward:
			BwdDeadZone *= 2.0f;
			break;
		default:
			break;
		}
	}
	
	if (FwdDeadZone + 45 >= AbsAngle)
	{
		return AE_CardinalDirection::Forward;
	}
	else if (135.0f - BwdDeadZone <= AbsAngle)
	{
		return AE_CardinalDirection::Backward;
	}
	else if (Angle < 0.0f)
	{
		return AE_CardinalDirection::Left;
	}
	else
	{
		return AE_CardinalDirection::Right;
	}
}

#pragma endregion

#pragma region TurnInPlace

void USFEnemyAnimInstance::UpdateTurnInPlace(float DeltaSeconds)
{
	// 현재 회전과 이전 회전의 Yaw 차이 계산
	float CurrentYaw = CachedRotation.Yaw;
	float PreviousYaw = PreviousRotation.Yaw;
	float DeltaYaw = FMath::FindDeltaAngleDegrees(PreviousYaw, CurrentYaw);

	// 이동 중에는 BlendOut
	if (bHasVelocity)
	{
		if (RootYawOffsetMode != ERootYawOffsetMode::BlendOut)
		{
			RootYawOffsetMode = ERootYawOffsetMode::BlendOut;
			bIsTurningInPlace = false;
		}
		ProcessBlendOutMode(DeltaSeconds);
		return;
	}

	// 정지 상태에서 모드별 처리
	switch (RootYawOffsetMode)
	{
		case ERootYawOffsetMode::Accumulate:
			ProcessAccumulateMode(DeltaYaw);
			break;

		case ERootYawOffsetMode::Hold:
			ProcessHoldMode();
			break;

		case ERootYawOffsetMode::BlendOut:
			ProcessBlendOutMode(DeltaSeconds);
			break;
	}
}

void USFEnemyAnimInstance::ProcessAccumulateMode(float DeltaYaw)
{
	// Yaw 변화를 반대 방향으로 누적 (시각적으로 제자리에 있는 것처럼)
	RootYawOffset += (-DeltaYaw);
	RootYawOffset = NormalizeAxis(RootYawOffset);

	// 임계값 초과 시 Turn In Place 애니메이션 트리거
	if (FMath::Abs(RootYawOffset) > TurnInPlaceThreshold)
	{
		bIsTurningInPlace = true;
		TurnDirection = RootYawOffset > 0.0f ? 1.0f : -1.0f; 
		
		RootYawOffsetMode = ERootYawOffsetMode::Hold;
	}
}

void USFEnemyAnimInstance::ProcessHoldMode()
{
	// 현재 RootYawOffset 값 유지
	// 애니메이션이 재생되는 동안 NativeUpdateAnimation에서 
	// RemainingTurnYaw 커브를 통해 값을 줄여나감
}

void USFEnemyAnimInstance::ProcessBlendOutMode(float DeltaSeconds)
{
	// RootYawOffset을 0으로 부드럽게 보간
	RootYawOffset = FMath::FInterpTo(RootYawOffset, 0.0f, DeltaSeconds, BlendOutSpeed);

	// 거의 0에 가까우면 완전히 0으로 설정하고 Accumulate 모드로 복귀
	if (FMath::Abs(RootYawOffset) < 0.1f)
	{
		RootYawOffset = 0.0f;
		bIsTurningInPlace = false;
		RootYawOffsetMode = ERootYawOffsetMode::Accumulate;
	}
}

void USFEnemyAnimInstance::ProcessRemainingTurnYaw(float DeltaTurnYaw)
{
	// Turn In Place 애니메이션의 커브 데이터만큼 RootYawOffset 감소
	// 커브가 90 -> 0으로 감소하면, Delta는 양수가 되므로 RootYawOffset도 감소
	RootYawOffset -= DeltaTurnYaw;
	RootYawOffset = NormalizeAxis(RootYawOffset);
}

void USFEnemyAnimInstance::OnTurnInPlaceCompleted()
{
	// AnimNotify에서 호출됨
	// BlendOut 모드로 전환
	RootYawOffsetMode = ERootYawOffsetMode::BlendOut;
	bIsTurningInPlace = false;
}

float USFEnemyAnimInstance::NormalizeAxis(float Angle)
{
	// -180 ~ 180 범위로 정규화
	Angle = FMath::Fmod(Angle, 360.0f);

	if (Angle > 180.0f)
	{
		Angle -= 360.0f;
	}
	else if (Angle < -180.0f)
	{
		Angle += 360.0f;
	}

	return Angle;
}

#pragma endregion

#pragma region Spring (Optional)

void USFEnemyAnimInstance::ApplySpringToRootYawOffset(float DeltaSeconds)
{
	// Spring을 적용하여 부드러운 보간 (선택사항)
	// RootYawOffset을 목표값으로 Spring 시뮬레이션
	
	float SpringTarget = RootYawOffset;

	SpringCurrentValue = SpringInterpolate(
		SpringCurrentValue,
		SpringTarget,
		DeltaSeconds,
		SpringVelocity,
		SpringStiffness,
		SpringDampingRatio,
		SpringMass
	);

	// 최종 출력값 (애니메이션 그래프에서 사용)
	SmoothedRootYawOffset = SpringCurrentValue;
	SmoothedRootYawOffset = NormalizeAxis(SmoothedRootYawOffset);
}

float USFEnemyAnimInstance::SpringInterpolate(float Current, float Target, float DeltaTime,
                                               float& Velocity, float Stiffness,
                                               float DampingRatio, float Mass)
{
	// Spring 물리 시뮬레이션
	// F = -k * x - c * v
	// a = F / m
	// v = v + a * dt
	// x = x + v * dt

	if (DeltaTime <= 0.0f || Mass <= 0.0f)
	{
		return Current;
	}

	// 오차 계산
	float Error = Target - Current;

	// Spring 힘 계산
	float SpringForce = Stiffness * Error;

	// Damping 힘 계산
	float DampingForce = 2.0f * DampingRatio * FMath::Sqrt(Stiffness * Mass) * Velocity;

	// 총 힘
	float TotalForce = SpringForce - DampingForce;

	// 가속도
	float Acceleration = TotalForce / Mass;

	// 속도 업데이트
	Velocity += Acceleration * DeltaTime;

	// 위치 업데이트
	float NewValue = Current + Velocity * DeltaTime;

	return NewValue;
}

#pragma endregion