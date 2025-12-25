// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/Enemy/SFEnemyAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/SFCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AnimCharacterMovementLibrary.h"
#include "KismetAnimationLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "AI/Controller/SFBaseAIController.h"
#include "Character/SFCharacterGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFEnemyAnimInstance)

USFEnemyAnimInstance::USFEnemyAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Character(nullptr)
	, CachedMovementComponent(nullptr)
	, CachedAIController(nullptr)
	, CachedLocation(FVector::ZeroVector)
	, PreviousWorldLocation(FVector::ZeroVector)
	, bIsFirstUpdate(true)
	, CachedRotation(FRotator::ZeroRotator)
	, CachedWorldVelocity(FVector::ZeroVector)
	, CachedWorldVelocity2D(FVector::ZeroVector)
	, CachedWorldAcceleration2D(FVector::ZeroVector)
	, PreviousWorldVelocity2D(FVector::ZeroVector)
	, PreviousRotation(FRotator::ZeroRotator)
	, CachedDeltaSeconds(0.0f)
	, CachedControlRotationYaw(0.0f)
	, CachedRotationMode(EAIRotationMode::None)
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
	, PreviousRemainingTurnYaw(0.0f)
	, TurnAngle(90.0f)
	, bIsForcedTurn (false)
{
}

#pragma region AnimationFunction

void USFEnemyAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* ASC)
{
	check(ASC);
	CachedAbilitySystemComponent = ASC;
	GameplayTagPropertyMap.Initialize(this, ASC);
}

bool USFEnemyAnimInstance::RequestTurnInPlace(float TargetYaw, bool bForceImmediate)
{
	if (!Character)
		return false;

	float CurrentYaw = CachedRotation.Yaw;
	float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw);
	float AbsDeltaYaw = FMath::Abs(DeltaYaw);

	if (AbsDeltaYaw < 15.0f)
	{
		return false;
	}

	if (bIsTurningInPlace && !bForceImmediate)
	{
		return false;
	}

	if (bIsTurningInPlace)
	{
		RootYawOffsetMode = ERootYawOffsetMode::BlendOut;
		bIsTurningInPlace = false;

		if (bForceImmediate)
		{
			RootYawOffset = 0.0f;
			RootYawOffsetMode = ERootYawOffsetMode::Accumulate;
		}
	}

	if (AbsDeltaYaw >= TurnInPlaceThreshold_180)
	{
		TurnAngle = 180.0f;
	}
	else
	{
		TurnAngle = 90.0f;
	}

	TurnDirection = DeltaYaw > 0.0f ? 1.0f : -1.0f;
	ActualTurnYaw = DeltaYaw;

	bIsTurningInPlace = true;
	bIsForcedTurn = true;
	RootYawOffsetMode = ERootYawOffsetMode::Hold;
	PreviousRemainingTurnYaw = TurnAngle;

	return true;
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

			if (AController* Controller = Character->GetController())
			{
				CachedAIController = Cast<ASFBaseAIController>(Controller);
			}
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

	if (!Character)
	{
		Character = Cast<ACharacter>(GetOwningActor());
		if (Character && !CachedMovementComponent)
		{
			CachedMovementComponent = Character->GetCharacterMovement();
		}
	}

	if (Character && !CachedAIController)
	{
		if (AController* Controller = Character->GetController())
		{
			CachedAIController = Cast<ASFBaseAIController>(Controller);
		}
	}

	if (!Character || !CachedMovementComponent)
	{
		return;
	}

	CachedDeltaSeconds = DeltaSeconds;

	if (CachedAIController)
	{
		CachedControlRotationYaw = CachedAIController->GetControlRotation().Yaw;
		CachedRotationMode = CachedAIController->GetCurrentRotationMode();
	}
	else
	{
		CachedControlRotationYaw = 0.0f;
		CachedRotationMode = EAIRotationMode::None;
	}

	if (!bIsFirstUpdate)
	{
		PreviousWorldLocation = CachedLocation;
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

	CachedWorldVelocity = CachedMovementComponent->Velocity;
	CachedWorldVelocity2D = FVector(CachedWorldVelocity.X, CachedWorldVelocity.Y, 0.0f);

	const FVector Acceleration = CachedMovementComponent->GetCurrentAcceleration();
	CachedWorldAcceleration2D = FVector(Acceleration.X, Acceleration.Y, 0.0f);

	// RemainingTurnYaw 커브 기반 TurnInPlace 처리
	if (bIsTurningInPlace)
	{
		float CurrentRemainingYaw = GetCurveValue(FName("RemainingTurnYaw"));
		float DeltaYaw = PreviousRemainingTurnYaw - CurrentRemainingYaw;

		if (FMath::Abs(DeltaYaw) > UE_SMALL_NUMBER)
		{
			ProcessRemainingTurnYaw(DeltaYaw);
		}

		PreviousRemainingTurnYaw = CurrentRemainingYaw;

		if (FMath::Abs(CurrentRemainingYaw) < 0.5f)
		{
			OnTurnInPlaceAnimationComplete();
		}
	}
	else
	{
		PreviousRemainingTurnYaw = TurnAngle;
	}

	UpdateTurnInPlace(DeltaSeconds);

	if (CachedAbilitySystemComponent)
	{
		bUsingAbility = CachedAbilitySystemComponent->HasMatchingGameplayTag(SFGameplayTags::Character_State_UsingAbility);
	}
}

void USFEnemyAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	UpdateLocationData(DeltaSeconds);
	UpdateRotationData();
	UpdateVelocityData();
	UpdateAccelerationData();
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
	DisplacementSinceLastUpdate = FVector::Dist(WorldLocation, PreviousWorldLocation);

	
	float TargetSpeed = 0.0f;
	if (DeltaSeconds > 0.0f)
	{
		TargetSpeed = DisplacementSinceLastUpdate / DeltaSeconds;
	}
	
	DisplacementSpeed = FMath::FInterpTo(DisplacementSpeed, TargetSpeed, DeltaSeconds, 5.0f);
}

void USFEnemyAnimInstance::UpdateRotationData()
{
	// TurnInPlace 중에는 ControlRotation 기준, 그 외에는 ActorRotation 기준
	if (bIsTurningInPlace)
	{
		WorldRotation = FRotator(0.f, CachedControlRotationYaw, 0.f);
	}
	else
	{
		WorldRotation = CachedRotation;
	}
}


void USFEnemyAnimInstance::UpdateVelocityData()
{
	WorldVelocity2D = CachedWorldVelocity2D;

	// TurnInPlace 중에는 ControlRotation 기준으로 Local 좌표계 계산
	FRotator RotationForLocalCalc = CachedRotation;
	if (bIsTurningInPlace)
	{
		RotationForLocalCalc = FRotator(0.f, CachedControlRotationYaw, 0.f);
	}

	FVector TargetLocalVelocity = RotationForLocalCalc.UnrotateVector(WorldVelocity2D);

	const float VelocityLength = TargetLocalVelocity.Size();
	bHasVelocity = VelocityLength > 1.0f;

	float TargetAngle = FMath::RadiansToDegrees(FMath::Atan2(TargetLocalVelocity.Y, TargetLocalVelocity.X));

	if (bHasVelocity)
	{
		LocalVelocity2D = FMath::VInterpTo(LocalVelocity2D, TargetLocalVelocity, CachedDeltaSeconds, 5.0f);

		float AngleDelta = FMath::FindDeltaAngleDegrees(LocalVelocityDirectionAngle, TargetAngle);
		float AdjustedTarget = LocalVelocityDirectionAngle + AngleDelta;

		LocalVelocityDirectionAngle = FMath::FInterpTo(
			LocalVelocityDirectionAngle,
			AdjustedTarget,
			CachedDeltaSeconds,
			5.0f
		);

		LocalVelocityDirectionAngle = FMath::UnwindDegrees(LocalVelocityDirectionAngle);
	}
	else
	{
		LocalVelocity2D = TargetLocalVelocity;
		LocalVelocityDirectionAngle = TargetAngle;
	}

	LocalVelocityDirection = GetCardinalDirectionFromAngle(
		LocalVelocityDirectionAngle,
		CardinalDirectionDeadZone,
		LocalVelocityDirection,
		bWasMovingLastFrame
	);

	bWasMovingLastFrame = bHasVelocity;
}

void USFEnemyAnimInstance::UpdateAccelerationData()
{
	// 서버는 실제 Acceleration, 클라이언트는 Velocity 변화량으로 추정
	if (GetOwningActor() && GetOwningActor()->HasAuthority())
	{
		WorldAcceleration2D = CachedWorldAcceleration2D;
	}
	else
	{
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

	LocalAcceleration2D = CachedRotation.UnrotateVector(WorldAcceleration2D);

	const float AccelerationLength = LocalAcceleration2D.Size();
	bHasAcceleration = AccelerationLength > 1.0f;

	PreviousWorldVelocity2D = CachedWorldVelocity2D;
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
	// None 모드: Ability 중이므로 RootYawOffset 동결
	if (CachedRotationMode == EAIRotationMode::None)
	{
		return;
	}

	// MovementDirection 모드: 이동 방향으로 회전하므로 TurnInPlace 불필요
	if (CachedRotationMode == EAIRotationMode::MovementDirection)
	{
		if (bIsTurningInPlace || FMath::Abs(RootYawOffset) > 0.1f)
		{
			ResetTurnInPlaceState();
		}
		return;
	}

	if (CachedRotationMode != EAIRotationMode::ControllerYaw &&
		CachedRotationMode != EAIRotationMode::TurnInPlace)
	{
		return;
	}

	float CurrentYaw = CachedRotation.Yaw;
	float TargetYaw = CachedControlRotationYaw;
	float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw);
	float AbsDeltaYaw = FMath::Abs(DeltaYaw);

	// 이동 중이면 BlendOut
	if (bHasVelocity)
	{
		if (RootYawOffsetMode != ERootYawOffsetMode::BlendOut)
		{
			RootYawOffsetMode = ERootYawOffsetMode::BlendOut;
			bIsTurningInPlace = false;
			bIsForcedTurn = false;
		}
	}
	else if (RootYawOffsetMode == ERootYawOffsetMode::BlendOut && !bIsTurningInPlace)
	{
		if (FMath::Abs(RootYawOffset) < 5.0f)
		{
			RootYawOffsetMode = ERootYawOffsetMode::Accumulate;
			bIsForcedTurn = false;
		}
	}

	// TurnInPlace 자동 종료
	if (bIsTurningInPlace && !bIsForcedTurn && AbsDeltaYaw < 15.0f)
	{
		if (CachedAIController && CachedRotationMode == EAIRotationMode::TurnInPlace)
		{
			CachedAIController->SetRotationMode(EAIRotationMode::ControllerYaw);
		}

		RootYawOffsetMode = ERootYawOffsetMode::BlendOut;
		bIsTurningInPlace = false;
		bIsForcedTurn = false;
	}

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
	// RootYawOffset에 DeltaYaw 누적
	RootYawOffset += DeltaYaw;
	RootYawOffset = FMath::ClampAngle(RootYawOffset, -180.0f, 180.0f);

	float AbsOffset = FMath::Abs(RootYawOffset);
	const float LowThreshold = 30.0f;

	// 임계값 초과 시 TurnInPlace 트리거
	if (AbsOffset > LowThreshold)
	{
		if (AbsOffset >= TurnInPlaceThreshold_180)
		{
			TurnAngle = 180.0f;
		}
		else
		{
			TurnAngle = 90.0f;
		}

		TurnDirection = RootYawOffset > 0.0f ? -1.0f : 1.0f;
		ActualTurnYaw = RootYawOffset;

		bIsTurningInPlace = true;
		bIsForcedTurn = false;
		RootYawOffsetMode = ERootYawOffsetMode::Hold;
		PreviousRemainingTurnYaw = TurnAngle;

		if (CachedAIController && CachedRotationMode == EAIRotationMode::ControllerYaw)
		{
			CachedAIController->SetRotationMode(EAIRotationMode::TurnInPlace);
		}
	}
}

void USFEnemyAnimInstance::ProcessHoldMode()
{
	// RemainingTurnYaw 커브로 RootYawOffset 감소
	float CurrentRemainingYaw = GetCurveValue(FName("RemainingTurnYaw"));
	float DeltaYaw = PreviousRemainingTurnYaw - CurrentRemainingYaw;

	RootYawOffset -= DeltaYaw * TurnDirection;
	PreviousRemainingTurnYaw = CurrentRemainingYaw;
}

void USFEnemyAnimInstance::ProcessBlendOutMode(float DeltaSeconds)
{
	RootYawOffset = FMath::FInterpTo(RootYawOffset, 0.0f, DeltaSeconds, BlendOutSpeed);

	if (FMath::Abs(RootYawOffset) < 0.1f)
	{
		RootYawOffset = 0.0f;
		bIsTurningInPlace = false;
		RootYawOffsetMode = ERootYawOffsetMode::Accumulate;
	}
}

void USFEnemyAnimInstance::ProcessRemainingTurnYaw(float DeltaTurnYaw)
{
	if (!bIsTurningInPlace)
		return;

	if (FMath::Abs(RootYawOffset) < 5.0f)
	{
		OnTurnInPlaceAnimationComplete();
	}
}

void USFEnemyAnimInstance::OnTurnInPlaceAnimationComplete()
{
	if (CachedAIController)
	{
		CachedAIController->SetRotationMode(EAIRotationMode::ControllerYaw);
	}

	bIsTurningInPlace = false;
	bIsForcedTurn = false;
	RootYawOffset = 0.0f;
	RootYawOffsetMode = ERootYawOffsetMode::Accumulate;
}

void USFEnemyAnimInstance::OnTurnInPlaceCompleted()
{
	OnTurnInPlaceAnimationComplete();
}

void USFEnemyAnimInstance::ResetTurnInPlaceState()
{
	bIsTurningInPlace = false;
	bIsForcedTurn = false;
	RootYawOffset = 0.0f;
	RootYawOffsetMode = ERootYawOffsetMode::Accumulate;
	TurnDirection = 0.0f;
	TurnAngle = 90.0f;
	ActualTurnYaw = 0.0f;
	PreviousRemainingTurnYaw = 90.0f;
}


#pragma endregion

