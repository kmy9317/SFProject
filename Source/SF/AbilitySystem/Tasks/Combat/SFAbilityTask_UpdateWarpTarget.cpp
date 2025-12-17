#include "SFAbilityTask_UpdateWarpTarget.h"

#include "AbilitySystemComponent.h"
#include "SFLogChannels.h"
#include "Character/SFCharacterBase.h"
#include "Combat/SFCombatTags.h"

USFAbilityTask_UpdateWarpTarget::USFAbilityTask_UpdateWarpTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = true;
	TargetName = NAME_None;
	Range = 200.f;
	InterpSpeed = 10.f;
	MaxAngle = 90.f;

	CurrentWarpDirection = FVector::ForwardVector;
	CurrentWarpLocation = FVector::ZeroVector;
	InitialForwardDirection = FVector::ForwardVector;

	bIsPaused = true; 

}

USFAbilityTask_UpdateWarpTarget* USFAbilityTask_UpdateWarpTarget::CreateTask(UGameplayAbility* OwningAbility, FName WarpTargetName, float WeaponRange, float RotationInterpSpeed, float MaxTurnAngle)
{
	USFAbilityTask_UpdateWarpTarget* Task = NewAbilityTask<USFAbilityTask_UpdateWarpTarget>(OwningAbility);
	Task->TargetName = WarpTargetName;
	Task->Range = WeaponRange;
	Task->InterpSpeed = RotationInterpSpeed;
	Task->MaxAngle = MaxTurnAngle;
	return Task;
}

void USFAbilityTask_UpdateWarpTarget::Activate()
{
	Super::Activate();

	AActor* AvatarActor = GetAvatarActor();
	if (!AvatarActor)
	{
		EndTask();
		return;
	}

	OwnerCharacter = Cast<ASFCharacterBase>(AvatarActor);
	if (!OwnerCharacter.IsValid())
	{
		EndTask();
		return;
	}

	MotionWarpingComp = OwnerCharacter->GetMotionWarpingComponent();
	if (!MotionWarpingComp.IsValid())
	{
		EndTask();
		return;
	}

	// 초기 방향 설정
	InitialForwardDirection = OwnerCharacter->GetActorForwardVector();
	CurrentWarpDirection = InitialForwardDirection;
	CurrentWarpLocation = OwnerCharacter->GetActorLocation() + (CurrentWarpDirection * Range);

	ApplyWarpTarget(CurrentWarpLocation, CurrentWarpDirection.Rotation());

	bIsPaused = !IsInWindupPhase();
}

void USFAbilityTask_UpdateWarpTarget::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!OwnerCharacter.IsValid())
	{
		EndTask();
		return;
	}

	const bool bInWindup = IsInWindupPhase();

	// Case 1: 업데이트 중 → 일시정지 (Windup 종료)
	if (!bIsPaused && !bInWindup)
	{
		bIsPaused = true;
		
		// 일시정지 델리게이트 브로드캐스트
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			// 방향 전환
			OnWarpTargetCommitted.Broadcast(CurrentWarpDirection, CurrentWarpLocation);
		}
		return;
	}
	// Case 2: 일시정지 → 재개 (Windup 재진입)
	if (bIsPaused && bInWindup)
	{
		bIsPaused = false;

		// 재진입 시 초기 방향을 현재 캐릭터 전방으로 리셋
		ResetInitialDirection();
	}
	// Case 3: 일시정지 상태 유지
	if (bIsPaused)
	{
		return;
	}
	// Case 4: Windup 중 - 방향 업데이트
	if (LockedTarget.IsValid())
	{
		UpdateWarpTargetFromLockedTarget(DeltaTime);
	}
	else
	{
		UpdateWarpTargetFromInput(DeltaTime);
	}
}

void USFAbilityTask_UpdateWarpTarget::ResetInitialDirection()
{
	if (OwnerCharacter.IsValid())
	{
		InitialForwardDirection = OwnerCharacter->GetActorForwardVector();
		CurrentWarpDirection = InitialForwardDirection;
		CurrentWarpLocation = OwnerCharacter->GetActorLocation() + (CurrentWarpDirection * Range);

		ApplyWarpTarget(CurrentWarpLocation, CurrentWarpDirection.Rotation());
	}
}

void USFAbilityTask_UpdateWarpTarget::UpdateWarpTargetFromInput(float DeltaTime)
{
	FVector InputDirection = GetPlayerInputDirection();

	// 입력이 없으면 현재 방향 유지
	if (InputDirection.IsNearlyZero())
	{
		InputDirection = CurrentWarpDirection;
	}

	// 최대 회전 각도 제한
	if (MaxAngle > 0.f && MaxAngle < 180.f)
	{
		InputDirection = ClampDirectionToMaxAngle(InputDirection);
	}

	// 방향 보간
	FVector NewDirection = InterpolateDirection(CurrentWarpDirection, InputDirection, DeltaTime);
	NewDirection.Z = 0.f;
	NewDirection.Normalize();

	// Warp 위치 계산
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector NewLocation = CharacterLocation + (NewDirection * Range);

	CurrentWarpDirection = NewDirection;
	CurrentWarpLocation = NewLocation;

	ApplyWarpTarget(NewLocation, NewDirection.Rotation());
}

void USFAbilityTask_UpdateWarpTarget::UpdateWarpTargetFromLockedTarget(float DeltaTime)
{
	if (!LockedTarget.IsValid())
	{
		UpdateWarpTargetFromInput(DeltaTime);
		return;
	}

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = LockedTarget->GetActorLocation();
	FVector DirectionToTarget = (TargetLocation - CharacterLocation).GetSafeNormal2D();

	// 최대 회전 각도 제한
	if (MaxAngle > 0.f && MaxAngle < 180.f)
	{
		DirectionToTarget = ClampDirectionToMaxAngle(DirectionToTarget);
	}

	// 방향 보간
	FVector NewDirection = InterpolateDirection(CurrentWarpDirection, DirectionToTarget, DeltaTime);
	NewDirection.Z = 0.f;
	NewDirection.Normalize();

	// Warp 위치 계산
	float DistanceToTarget = FVector::Dist2D(CharacterLocation, TargetLocation);
	FVector NewLocation;
	
	if (DistanceToTarget <= Range)
	{
		NewLocation = TargetLocation;
	}
	else
	{
		NewLocation = CharacterLocation + (NewDirection * Range);
	}

	CurrentWarpDirection = NewDirection;
	CurrentWarpLocation = NewLocation;

	ApplyWarpTarget(NewLocation, NewDirection.Rotation());
}

FVector USFAbilityTask_UpdateWarpTarget::ClampDirectionToMaxAngle(const FVector& DesiredDirection) const
{
	if (!OwnerCharacter.IsValid())
	{
		return DesiredDirection;
	}

	float DotProduct = FVector::DotProduct(InitialForwardDirection.GetSafeNormal2D(), DesiredDirection.GetSafeNormal2D());
	float AngleRadians = FMath::Acos(FMath::Clamp(DotProduct, -1.f, 1.f));
	float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	if (AngleDegrees <= MaxAngle)
	{
		return DesiredDirection;
	}

	// 좌우 방향 결정 후 클램프
	FVector CrossProduct = FVector::CrossProduct(InitialForwardDirection, DesiredDirection);
	float Sign = FMath::Sign(CrossProduct.Z);

	FRotator ClampedRotation = InitialForwardDirection.Rotation();
	ClampedRotation.Yaw += MaxAngle * Sign;

	return ClampedRotation.Vector().GetSafeNormal2D();
}

FVector USFAbilityTask_UpdateWarpTarget::InterpolateDirection(const FVector& CurrentDir, const FVector& TargetDir, float DeltaTime) const
{
	if (InterpSpeed <= 0.f)
	{
		return TargetDir;
	}

	FRotator CurrentRotation = CurrentDir.Rotation();
	FRotator TargetRotation = TargetDir.Rotation();
	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, InterpSpeed);

	return NewRotation.Vector();
}

void USFAbilityTask_UpdateWarpTarget::ApplyWarpTarget(const FVector& Location, const FRotator& Rotation)
{
	if (!MotionWarpingComp.IsValid())
	{
		return;
	}

	MotionWarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(TargetName, Location, Rotation);
}

FVector USFAbilityTask_UpdateWarpTarget::GetPlayerInputDirection() const
{
	if (!OwnerCharacter.IsValid())
	{
		return FVector::ForwardVector;
	}

	FVector InputDirection = OwnerCharacter->GetLastInputDirection();

	if (InputDirection.IsNearlyZero())
	{
		return OwnerCharacter->GetActorForwardVector();
	}

	return InputDirection.GetSafeNormal2D();
}

bool USFAbilityTask_UpdateWarpTarget::IsInWindupPhase() const
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC)
	{
		return false;
	}

	return ASC->HasMatchingGameplayTag(SFGameplayTags::Combat_Phase_Windup);
}

void USFAbilityTask_UpdateWarpTarget::SetLockedTarget(AActor* InTarget)
{
	LockedTarget = InTarget;
	UE_LOG(LogSF, Warning, TEXT("[WarpTargetTask] Locked Target set: %s"), *GetNameSafe(InTarget));
}

void USFAbilityTask_UpdateWarpTarget::ClearLockedTarget()
{
	LockedTarget = nullptr;
	UE_LOG(LogSF, Warning, TEXT("[WarpTargetTask] Locked Target cleared"));
}
