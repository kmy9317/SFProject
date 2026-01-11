#include "SFAbilityTask_UpdateWarpTarget.h"

#include "AbilitySystemComponent.h"
#include "SFLogChannels.h"
#include "Character/SFCharacterBase.h"
#include "Character/Hero/Component/SFHeroMovementComponent.h"
#include "Character/Hero/Component/SFLockOnComponent.h"
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
	if (!LockedTarget.IsValid())
	{
		if (USFLockOnComponent* LockOnComp = OwnerCharacter->FindComponentByClass<USFLockOnComponent>())
		{
			if (AActor* TargetFromComp = LockOnComp->GetCurrentTarget())
			{
				SetLockedTarget(TargetFromComp);
			}
		}
	}

	if (LockedTarget.IsValid())
	{
		FVector DirToTarget = (LockedTarget->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal2D();
        
		if (DirToTarget.IsNearlyZero())
		{
			InitialForwardDirection = OwnerCharacter->GetActorForwardVector();
		}
		else
		{
			InitialForwardDirection = DirToTarget;
		}
	}
	else
	{
		InitialForwardDirection = OwnerCharacter->GetActorForwardVector();
	}
	
	CurrentWarpDirection = InitialForwardDirection;
	CurrentWarpLocation = OwnerCharacter->GetActorLocation() + (CurrentWarpDirection * Range);

	ApplyWarpTargetWithSync(CurrentWarpLocation, CurrentWarpDirection.Rotation());

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
	// 매 틱마다 LockOnComponent에서 락온 타겟을 확인 (락온 타겟이 변경될 수 있음)
	if (USFLockOnComponent* LockOnComp = OwnerCharacter->FindComponentByClass<USFLockOnComponent>())
	{
		if (AActor* TargetFromComp = LockOnComp->GetCurrentTarget())
		{
			// 락온 타겟이 변경되었거나 처음 설정되는 경우
			if (!LockedTarget.IsValid() || LockedTarget != TargetFromComp)
			{
				SetLockedTarget(TargetFromComp);
			}
			UpdateWarpTargetFromLockedTarget(DeltaTime);
		}
		else
		{
			// 락온 타겟이 해제된 경우
			if (LockedTarget.IsValid())
			{
				ClearLockedTarget();
			}
			UpdateWarpTargetFromInput(DeltaTime);
		}
	}
	else
	{
		// LockOnComponent가 없는 경우
		if (LockedTarget.IsValid())
		{
			ClearLockedTarget();
		}
		UpdateWarpTargetFromInput(DeltaTime);
	}
}

void USFAbilityTask_UpdateWarpTarget::OnDestroy(bool AbilityEndedOrCancelled)
{
	// CMC의 Warp 타겟 해제
	if (OwnerCharacter.IsValid())
	{
		if (USFHeroMovementComponent* SFMovement = Cast<USFHeroMovementComponent>(OwnerCharacter->GetCharacterMovement()))
		{
			SFMovement->ClearWarpTarget();
		}
	}
	
	Super::OnDestroy(AbilityEndedOrCancelled);
}

void USFAbilityTask_UpdateWarpTarget::ResetInitialDirection()
{
	if (OwnerCharacter.IsValid())
	{
		if (LockedTarget.IsValid())
		{
			FVector DirToTarget = (LockedTarget->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal2D();
			if (!DirToTarget.IsNearlyZero())
			{
				InitialForwardDirection = DirToTarget;
			}
			else
			{
				InitialForwardDirection = OwnerCharacter->GetActorForwardVector();
			}
		}
		else
		{
			InitialForwardDirection = OwnerCharacter->GetActorForwardVector();
		}

		CurrentWarpDirection = InitialForwardDirection;
		CurrentWarpLocation = OwnerCharacter->GetActorLocation() + (CurrentWarpDirection * Range);

		ApplyWarpTargetWithSync(CurrentWarpLocation, CurrentWarpDirection.Rotation());
	}
}

void USFAbilityTask_UpdateWarpTarget::UpdateWarpTargetFromInput(float DeltaTime)
{
	// 락온 타겟이 있는지 다시 확인 (타이밍 이슈 대비)
	if (USFLockOnComponent* LockOnComp = OwnerCharacter->FindComponentByClass<USFLockOnComponent>())
	{
		if (AActor* TargetFromComp = LockOnComp->GetCurrentTarget())
		{
			// 락온 타겟이 있으면 입력 방향 무시하고 락온 타겟 방향 사용
			if (!LockedTarget.IsValid() || LockedTarget != TargetFromComp)
			{
				SetLockedTarget(TargetFromComp);
			}
			UpdateWarpTargetFromLockedTarget(DeltaTime);
			return;
		}
	}
	
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

	ApplyWarpTargetWithSync(CurrentWarpLocation, CurrentWarpDirection.Rotation());
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
	
	// 락온 타겟이 있을 때는 입력 방향 무시하고 락온 타겟 방향만 사용 (각도 제한 없음)
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

	ApplyWarpTargetWithSync(NewLocation, NewDirection.Rotation());
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

void USFAbilityTask_UpdateWarpTarget::ApplyWarpTargetWithSync(const FVector& Location, const FRotator& Rotation)
{
	// MotionWarpingComponent에 적용
	ApplyWarpTarget(Location, Rotation);

	// CMC에 적용 (네트워크 동기화용)
	if (OwnerCharacter.IsValid())
	{
		if (USFHeroMovementComponent* SFMovement = Cast<USFHeroMovementComponent>(OwnerCharacter->GetCharacterMovement()))
		{
			SFMovement->SetWarpTarget(Location, Rotation);
		}
	}
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

	// 락온 타겟이 있으면 입력 방향을 반환하지 않음 (락온 타겟 방향 우선)
	if (USFLockOnComponent* LockOnComp = OwnerCharacter->FindComponentByClass<USFLockOnComponent>())
	{
		if (LockOnComp->GetCurrentTarget())
		{
			// 락온 타겟이 있으면 입력 방향 대신 캐릭터 전방 방향 반환 (입력 방향 무시)
			return OwnerCharacter->GetActorForwardVector();
		}
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
