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

	//이전 프레임 값 저장 
	if (!bIsFirstUpdate)
	{
		PreviousWorldLocation = CachedLocation;
	}
	else
	{
		PreviousWorldLocation = Character->GetActorLocation();
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
	
	if (VelocityLength >1.0f)
	{
		bHasVelocity = true;
	}
	else
	{
		bHasVelocity = false;
	}

	//로컬 좌표계 각도
	LocalVelocityDirectionAngle = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity2D.Y, LocalVelocity2D.X));

	LocalVelocityDirection = GetCardinalDirectionFromAngle(LocalVelocityDirectionAngle, CardinalDirectionDeadZone, LocalVelocityDirection,bWasMovingLastFrame);

	bWasMovingLastFrame = bHasVelocity;
}

void USFEnemyAnimInstance::UpdateAccelerationData()
{
	WorldAcceleration2D = CachedWorldAcceleration2D;
	
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

	//  Locomotion Library 함수 호출
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
		case AE_CardinalDirection::Forward :
			FwdDeadZone *= 2.0f;
			break;
		case AE_CardinalDirection::Backward :
			BwdDeadZone *= 2.0f;
			break;
		default:
			break;
		}
	}
	if (FwdDeadZone +45 >= AbsAngle)
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
