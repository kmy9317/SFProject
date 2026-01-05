// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/Enemy/SFEnemyAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/SFCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AnimCharacterMovementLibrary.h"
#include "KismetAnimationLibrary.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Kismet/KismetMathLibrary.h"
#include "AI/Controller/SFBaseAIController.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"

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
	, CachedRotationMode(EAIRotationMode::MovementDirection)
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
	CachedAbilitySystemComponent = ASC;
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
	

	if (!Character || !CachedMovementComponent) return;

	CachedDeltaSeconds = DeltaSeconds;

	// 데이터 업데이트
	if (CachedAIController)
	{
		CachedControlRotationYaw = CachedAIController->GetControlRotation().Yaw;
		CachedRotationMode = CachedAIController->GetCurrentRotationMode();
	}

	if (CachedAbilitySystemComponent)
	{
		bUsingAbility = CachedAbilitySystemComponent->HasMatchingGameplayTag(SFGameplayTags::Character_State_UsingAbility);
		bIsTurningInPlace = CachedAbilitySystemComponent->HasMatchingGameplayTag(SFGameplayTags::Character_State_TurningInPlace);
	}

	// 위치 및 회전 캐싱
	if (bIsFirstUpdate)
	{
		PreviousWorldLocation = Character->GetActorLocation();
		PreviousRotation = Character->GetActorRotation();
		bIsFirstUpdate = false;
	}
	else
	{
		PreviousWorldLocation = CachedLocation;
		PreviousRotation = CachedRotation;
	}

	CachedLocation = Character->GetActorLocation();
	CachedRotation = Character->GetActorRotation();
	CachedWorldAcceleration2D = CachedMovementComponent->GetCurrentAcceleration();

	if (CachedAIController)
	{
		
		CachedControlRotation = CachedAIController->GetControlRotation();
		CachedControlRotationYaw = CachedControlRotation.Yaw; 
		CachedRotationMode = CachedAIController->GetCurrentRotationMode();
	}
	UpdateAimOffsetData(DeltaSeconds);
	
	
}

void USFEnemyAnimInstance::UpdateAimOffsetData(float DeltaSeconds)
{
	FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CachedControlRotation, CachedRotation);
    
	const float TargetPitch = Delta.Pitch;
	const float TargetYaw = Delta.Yaw;
    
	
	bool bIsAttacking = false;
	float MaxYawOffset = 30.0f;  
	float MaxPitchOffset = 30.0f;
	float InterpSpeed = 10.0f;
    
	if (CachedAbilitySystemComponent)
	{
		bIsAttacking = 
			CachedAbilitySystemComponent->HasMatchingGameplayTag(SFGameplayTags::Ability_Dragon_FlameBreath_Line) ||
			CachedAbilitySystemComponent->HasMatchingGameplayTag(SFGameplayTags::Ability_Dragon_Bite);
        
		if (bIsAttacking)
		{
			MaxYawOffset = 90.0f;   
			MaxPitchOffset = 90.0f;
			InterpSpeed = 15.0f;     
		}
	}
    
	AimPitch = FMath::FInterpTo(AimPitch, TargetPitch, DeltaSeconds, InterpSpeed);
	AimYaw = FMath::FInterpTo(AimYaw, TargetYaw, DeltaSeconds, InterpSpeed);
    
	AimPitch = FMath::Clamp(AimPitch, -MaxPitchOffset, MaxPitchOffset);
	AimYaw = FMath::Clamp(AimYaw, -MaxYawOffset, MaxYawOffset);
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

	WorldRotation = CachedRotation;
	
}

void USFEnemyAnimInstance::UpdateVelocityData()
{
	if (!CachedMovementComponent) return;
    
	FVector Velocity = CachedMovementComponent->GetLastUpdateVelocity();
    
	WorldVelocity2D = FVector(Velocity.X, Velocity.Y, 0.0f);
    
	GroundSpeed = WorldVelocity2D.Size(); 
	FlySpeed = Velocity.Size();         

	bHasVelocity = (GroundSpeed > 3.0f) || (FlySpeed > 3.0f);
	
	FRotator RotationForLocalCalc = CachedRotation;
	if (bIsTurningInPlace)
	{
		RotationForLocalCalc = FRotator(0.f, CachedControlRotationYaw, 0.f);
	}

	if (bHasVelocity)
	{
		float TargetAngle = UKismetAnimationLibrary::CalculateDirection(WorldVelocity2D, RotationForLocalCalc);
		float AngleDelta = FMath::FindDeltaAngleDegrees(LocalVelocityDirectionAngle, TargetAngle);
		LocalVelocityDirectionAngle = FMath::FInterpTo(LocalVelocityDirectionAngle, LocalVelocityDirectionAngle + AngleDelta, CachedDeltaSeconds, 5.0f);
		LocalVelocityDirectionAngle = FMath::UnwindDegrees(LocalVelocityDirectionAngle);
	}
	else
	{
		LocalVelocityDirectionAngle = 0.0f; 
	}
	
	LocalVelocityDirection = GetCardinalDirectionFromAngle(LocalVelocityDirectionAngle, CardinalDirectionDeadZone, LocalVelocityDirection, bWasMovingLastFrame);
	
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

