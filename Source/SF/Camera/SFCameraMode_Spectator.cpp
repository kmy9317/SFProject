#include "SFCameraMode_Spectator.h"

#include "SFCameraComponent.h"
#include "Curves/CurveVector.h"
#include "Pawn/SFSpectatorPawn.h"
#include "Player/SFPlayerState.h"

AActor* USFCameraMode_Spectator::GetFollowTarget() const
{
	if (AActor* TargetActor = GetTargetActor())
	{
		if (ASFSpectatorPawn* Spectator = Cast<ASFSpectatorPawn>(TargetActor))
		{
			return Spectator->GetFollowTarget();
		}
	}
	return nullptr;
}

FVector USFCameraMode_Spectator::GetFollowTargetPivotLocation() const
{
	AActor* FollowTarget = GetFollowTarget();
	if (!FollowTarget)
	{
		return GetPivotLocation();
	}

	if (APawn* TargetPawn = Cast<APawn>(FollowTarget))
	{
		return TargetPawn->GetPawnViewLocation();
	}
	
	return FollowTarget->GetActorLocation();
}

FRotator USFCameraMode_Spectator::GetFollowTargetPivotRotation() const
{
	AActor* FollowTarget = GetFollowTarget();
	if (!FollowTarget)
	{
		return GetPivotRotation();
	}

	if (APawn* TargetPawn = Cast<APawn>(FollowTarget))
	{
		if (ASFPlayerState* PS = TargetPawn->GetPlayerState<ASFPlayerState>())
		{
			return PS->GetReplicatedViewRotation();
		}
		return TargetPawn->GetViewRotation();
	}
	
	return FollowTarget->GetActorRotation();
}

void USFCameraMode_Spectator::OnActivation()
{
	Super::OnActivation();

	SmoothedFollowTargetPivot = GetFollowTargetPivotLocation();
	SmoothedFollowTargetRotation = GetFollowTargetPivotRotation();
}

void USFCameraMode_Spectator::UpdateView(float DeltaTime)
{
	AActor* FollowTarget = GetFollowTarget();
	if (!FollowTarget)
	{
		Super::UpdateView(DeltaTime);
		return;
	}

	// FollowTarget의 원시 피벗 위치/회전
	const FVector RawPivotLocation = GetFollowTargetPivotLocation();
	const FRotator RawPivotRotation = GetFollowTargetPivotRotation();

	// 자체 보간 (네트워크 지터 흡수)
	SmoothedFollowTargetPivot = FMath::VInterpTo(
		SmoothedFollowTargetPivot,
		RawPivotLocation,
		DeltaTime,
		PivotFollowSpeed
	);

	SmoothedFollowTargetRotation = FMath::RInterpTo(
		SmoothedFollowTargetRotation,
		RawPivotRotation,
		DeltaTime,
		PivotFollowSpeed
	);

	FVector PivotLocation = SmoothedFollowTargetPivot;
	FRotator PivotRotation = SmoothedFollowTargetRotation;

	// Pitch 제한
	PivotRotation.Pitch = FMath::ClampAngle(PivotRotation.Pitch, ViewPitchMin, ViewPitchMax);

	// Yaw 제한
	if (IsYawLimitsActive())
	{
		float CharacterYaw = FollowTarget->GetActorRotation().Yaw;
		float RelativeYaw = FRotator::NormalizeAxis(PivotRotation.Yaw - CharacterYaw);
		float TargetRelativeYaw = FMath::Clamp(RelativeYaw, ViewYawMin, ViewYawMax);

		if (RelativeYaw < ViewYawMin || RelativeYaw > ViewYawMax)
		{
			bIsYawInterpolating = true;
		}

		float FinalRelativeYaw;
		if (bIsYawInterpolating)
		{
			FinalRelativeYaw = FMath::FInterpTo(PreviousRelativeYaw, TargetRelativeYaw, DeltaTime, YawLimitInterpSpeed);
			if (FMath::IsNearlyEqual(FinalRelativeYaw, TargetRelativeYaw, 1.0f))
			{
				bIsYawInterpolating = false;
				FinalRelativeYaw = TargetRelativeYaw;
			}
		}
		else
		{
			FinalRelativeYaw = TargetRelativeYaw;
		}

		PreviousRelativeYaw = FinalRelativeYaw;
		PivotRotation.Yaw = CharacterYaw + FinalRelativeYaw;
	}

	View.Location = PivotLocation;
	View.Rotation = PivotRotation;
	View.ControlRotation = View.Rotation;
	View.FieldOfView = FieldOfView;

	// 오프셋 커브 적용
	if (bUseRuntimeFloatCurves == false)
	{
		if (TargetOffsetCurve)
		{
			const FVector TargetOffset = TargetOffsetCurve->GetVectorValue(PivotRotation.Pitch);
			View.Location = PivotLocation + PivotRotation.RotateVector(TargetOffset);
		}
	}
	else
	{
		FVector TargetOffset(0.0f);
		TargetOffset.X = TargetOffsetX.GetRichCurveConst()->Eval(PivotRotation.Pitch);
		TargetOffset.Y = TargetOffsetY.GetRichCurveConst()->Eval(PivotRotation.Pitch);
		TargetOffset.Z = TargetOffsetZ.GetRichCurveConst()->Eval(PivotRotation.Pitch);
		View.Location = PivotLocation + PivotRotation.RotateVector(TargetOffset);
	}

	UpdatePreventPenetration(DeltaTime);

	if (USFCameraComponent* CameraComp = GetSFCameraComponent())
	{
		CameraComp->SetSharedPenetrationBlockedPct(AimLineToDesiredPosBlockedPct);
	}
}

void USFCameraMode_Spectator::UpdatePreventPenetration(float DeltaTime)
{
	if (!bPreventPenetration)
	{
		return;
	}

	AActor* TargetActor = GetTargetActor();
	if (!TargetActor)
	{
		return;
	}

	AActor* FollowTarget = GetFollowTarget();
	if (!FollowTarget)
	{
		Super::UpdatePreventPenetration(DeltaTime);
		return;
	}

	const UPrimitiveComponent* FollowTargetRoot = Cast<UPrimitiveComponent>(FollowTarget->GetRootComponent());
	if (!FollowTargetRoot)
	{
		Super::UpdatePreventPenetration(DeltaTime);
		return;
	}

	// === 1단계: FollowTarget 실제 위치 기준으로 충돌 계산 ===
	
	// 실제 피벗/카메라 위치 계산
	FVector RealPivot = GetFollowTargetPivotLocation();
	FVector RealCameraLocation = RealPivot;
	
	if (bUseRuntimeFloatCurves == false)
	{
		if (TargetOffsetCurve)
		{
			const FVector TargetOffset = TargetOffsetCurve->GetVectorValue(View.Rotation.Pitch);
			RealCameraLocation = RealPivot + View.Rotation.RotateVector(TargetOffset);
		}
	}
	else
	{
		FVector TargetOffset(0.0f);
		TargetOffset.X = TargetOffsetX.GetRichCurveConst()->Eval(View.Rotation.Pitch);
		TargetOffset.Y = TargetOffsetY.GetRichCurveConst()->Eval(View.Rotation.Pitch);
		TargetOffset.Z = TargetOffsetZ.GetRichCurveConst()->Eval(View.Rotation.Pitch);
		RealCameraLocation = RealPivot + View.Rotation.RotateVector(TargetOffset);
	}

	// 실제 캐릭터 루트 위치
	FVector RealRoot = FollowTarget->GetActorLocation();

	// SafeLocation 계산 (ThirdPerson과 동일)
	FVector ClosestPointOnLineToCapsuleCenter;
	FVector SafeLocation = RealRoot;
	FMath::PointDistToLine(SafeLocation, View.Rotation.Vector(), RealCameraLocation, ClosestPointOnLineToCapsuleCenter);

	float const PushInDistance = PenetrationAvoidanceFeelers[0].Extent + CollisionPushOutDistance;
	float const MaxHalfHeight = FollowTarget->GetSimpleCollisionHalfHeight() - PushInDistance;
	SafeLocation.Z = FMath::Clamp(ClosestPointOnLineToCapsuleCenter.Z, RealRoot.Z - MaxHalfHeight, RealRoot.Z + MaxHalfHeight);

	float DistanceSqr;
	FollowTargetRoot->GetSquaredDistanceToCollision(ClosestPointOnLineToCapsuleCenter, DistanceSqr, SafeLocation);

	if (PenetrationAvoidanceFeelers.Num() > 0)
	{
		SafeLocation += (SafeLocation - ClosestPointOnLineToCapsuleCenter).GetSafeNormal() * PushInDistance;
	}

	// SafeLocation의 루트 기준 로컬 오프셋 저장
	FVector SafeLocalOffset = SafeLocation - RealRoot;

	// 충돌 검사
	bool const bSingleRayPenetrationCheck = !bDoPredictiveAvoidance;
	PreventCameraPenetration(*FollowTarget, SafeLocation, RealCameraLocation, DeltaTime, AimLineToDesiredPosBlockedPct, bSingleRayPenetrationCheck);

	// === 2단계: 보간된 View.Location에 결과 적용 ===
	
	// 보간된 루트 위치 계산
	FVector SmoothedRoot;
	if (APawn* TargetPawn = Cast<APawn>(FollowTarget))
	{
		FVector EyeOffset = TargetPawn->GetPawnViewLocation() - RealRoot;
		SmoothedRoot = SmoothedFollowTargetPivot - EyeOffset;
	}
	else
	{
		SmoothedRoot = SmoothedFollowTargetPivot;
	}

	// 보간된 SafeLocation = 보간된 루트 + 동일한 로컬 오프셋
	FVector SmoothedSafeLocation = SmoothedRoot + SafeLocalOffset;

	// 원래 View.Location 저장 (커브 적용된 보간 카메라 위치)
	FVector OriginalViewLocation = View.Location;

	// 막힌 비율 적용
	if (AimLineToDesiredPosBlockedPct < (1.f - ZERO_ANIMWEIGHT_THRESH))
	{
		View.Location = SmoothedSafeLocation + (OriginalViewLocation - SmoothedSafeLocation) * AimLineToDesiredPosBlockedPct;
	}
}