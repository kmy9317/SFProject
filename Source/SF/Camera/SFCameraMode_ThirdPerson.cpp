// Copyright Epic Games, Inc. All Rights Reserved.

#include "SFCameraMode_ThirdPerson.h"
#include "Camera/SFCameraMode.h"
#include "Components/PrimitiveComponent.h"
#include "Camera/SFPenetrationAvoidanceFeeler.h"
#include "Curves/CurveVector.h"
#include "Engine/Canvas.h"
#include "GameFramework/CameraBlockingVolume.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Math/RotationMatrix.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFCameraMode_ThirdPerson)

namespace SFCameraMode_ThirdPerson_Statics
{
	// 카메라 충돌을 무시할 액터(물체)에 붙이는 태그 이름
	static const FName NAME_IgnoreCameraCollision = TEXT("IgnoreCameraCollision");
}

USFCameraMode_ThirdPerson::USFCameraMode_ThirdPerson()
{
	TargetOffsetCurve = nullptr;

	// 엘든링 스타일 '더듬이(Feeler)' 설정
	// (각도, 월드 가중치, 폰 가중치, 광선 크기, 성능 최적화용 추적 간격)
	
	// 메인 광선(Ray): 중앙, 가장 중요 (가중치 1.0)
	PenetrationAvoidanceFeelers.Add(FSFPenetrationAvoidanceFeeler(FRotator(+00.0f, +00.0f, 0.0f), 1.00f, 1.00f, 14.f, 0));
	// 좌우 예측 광선 (16도) (가중치 0.75)
	PenetrationAvoidanceFeelers.Add(FSFPenetrationAvoidanceFeeler(FRotator(+00.0f, +16.0f, 0.0f), 0.75f, 0.75f, 00.f, 3));
	PenetrationAvoidanceFeelers.Add(FSFPenetrationAvoidanceFeeler(FRotator(+00.0f, -16.0f, 0.0f), 0.75f, 0.75f, 00.f, 3));
	// 좌우 예측 광선 (32도) (가중치 0.50)
	PenetrationAvoidanceFeelers.Add(FSFPenetrationAvoidanceFeeler(FRotator(+00.0f, +32.0f, 0.0f), 0.50f, 0.50f, 00.f, 5));
	PenetrationAvoidanceFeelers.Add(FSFPenetrationAvoidanceFeeler(FRotator(+00.0f, -32.0f, 0.0f), 0.50f, 0.50f, 00.f, 5));
	// 상하 예측 광선
	PenetrationAvoidanceFeelers.Add(FSFPenetrationAvoidanceFeeler(FRotator(+20.0f, +00.0f, 0.0f), 1.00f, 1.00f, 00.f, 4));
	PenetrationAvoidanceFeelers.Add(FSFPenetrationAvoidanceFeeler(FRotator(-20.0f, +00.0f, 0.0f), 0.50f, 0.50f, 00.f, 4));
}

// 3인칭 카메라 뷰 업데이트
void USFCameraMode_ThirdPerson::UpdateView(float DeltaTime)
{
	// 캐릭터 상태(앉기, 점프)에 따른 오프셋(위치 보정) 계산
	UpdateForTarget(DeltaTime);
	UpdateCrouchOffset(DeltaTime);
	UpdateJumpOffset(DeltaTime);

	// 피벗 위치 = 기본 위치 + 앉기 보정값 + 점프 보정값
	FVector PivotLocation = GetPivotLocation() + CurrentCrouchOffset + CurrentJumpOffset;
	FRotator PivotRotation = GetPivotRotation();

	// Pitch(상하 각도) 제한
	PivotRotation.Pitch = FMath::ClampAngle(PivotRotation.Pitch, ViewPitchMin, ViewPitchMax);

	View.Location = PivotLocation;
	View.Rotation = PivotRotation;
	View.ControlRotation = View.Rotation;
	View.FieldOfView = FieldOfView;

	// Pitch(상하 각도)에 따라 카메라 거리(오프셋)를 조절하는 커브 적용
	if (bUseRuntimeFloatCurves == false)
	{
		// 커브 에셋 사용
		if (TargetOffsetCurve)
		{
			const FVector TargetOffset = TargetOffsetCurve->GetVectorValue(PivotRotation.Pitch);
			View.Location = PivotLocation + PivotRotation.RotateVector(TargetOffset);
		}
	}
	else
	{
		// 런타임 커브(X, Y, Z 각각) 사용
		FVector TargetOffset(0.0f);
		TargetOffset.X = TargetOffsetX.GetRichCurveConst()->Eval(PivotRotation.Pitch);
		TargetOffset.Y = TargetOffsetY.GetRichCurveConst()->Eval(PivotRotation.Pitch);
		TargetOffset.Z = TargetOffsetZ.GetRichCurveConst()->Eval(PivotRotation.Pitch);

		View.Location = PivotLocation + PivotRotation.RotateVector(TargetOffset);
	}

	// 최종 희망 카메라 위치를 조정하여 벽 뚫림(관통) 방지
	UpdatePreventPenetration(DeltaTime);
}

// 타겟(캐릭터) 상태 추적
void USFCameraMode_ThirdPerson::UpdateForTarget(float DeltaTime)
{
	if (const ACharacter* TargetCharacter = Cast<ACharacter>(GetTargetActor()))
	{
		// 앉기 상태 확인
		if (TargetCharacter->bIsCrouched)
		{
			const ACharacter* TargetCharacterCDO = TargetCharacter->GetClass()->GetDefaultObject<ACharacter>();
			const float CrouchedHeightAdjustment = TargetCharacterCDO->CrouchedEyeHeight - TargetCharacterCDO->BaseEyeHeight;

			SetTargetCrouchOffset(FVector(0.f, 0.f, CrouchedHeightAdjustment)); // 앉기 오프셋 설정
			return;
		}

		// 점프/낙하 상태 확인
		UCharacterMovementComponent* CharacterMovement = TargetCharacter->GetCharacterMovement();
		if (CharacterMovement && CharacterMovement->IsFalling())
		{
			SetTargetJumpOffset(JumpOffset); // 점프 오프셋 설정
			return;
		}
	}

	// 기본 상태: 모든 오프셋 0
	SetTargetCrouchOffset(FVector::ZeroVector);
	SetTargetJumpOffset(FVector::ZeroVector);
}

// 디버그 정보 표시 (충돌한 액터 목록 등)
void USFCameraMode_ThirdPerson::DrawDebug(UCanvas* Canvas) const
{
	Super::DrawDebug(Canvas);

#if ENABLE_DRAW_DEBUG
	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;
	for (int i = 0; i < DebugActorsHitDuringCameraPenetration.Num(); i++)
	{
		DisplayDebugManager.DrawString(
			FString::Printf(TEXT("HitActorDuringPenetration[%d]: %s")
				, i
				, *DebugActorsHitDuringCameraPenetration[i]->GetName()));
	}

	LastDrawDebugTime = GetWorld()->GetTimeSeconds();
#endif
}

// 벽 뚫림(관통) 방지 로직 업데이트
void USFCameraMode_ThirdPerson::UpdatePreventPenetration(float DeltaTime)
{
	if (!bPreventPenetration)
	{
		return;
	}

	AActor* TargetActor = GetTargetActor();
	// ...

	const UPrimitiveComponent* TargetRootComponent = Cast<UPrimitiveComponent>(TargetActor->GetRootComponent());
	if (TargetRootComponent)
	{
		// 엘든링 스타일: '안전 위치(SafeLocation)'를 캡슐 내부로 설정
		// (카메라가 벽에 막혔을 때 돌아갈 최소한의 위치)
		FVector ClosestPointOnLineToCapsuleCenter;
		FVector SafeLocation = TargetActor->GetActorLocation();
		FMath::PointDistToLine(SafeLocation, View.Rotation.Vector(), View.Location, ClosestPointOnLineToCapsuleCenter);

		// '안전 위치'가 캐릭터 캡슐 높이 범위 내에 있도록 조정
		float const PushInDistance = PenetrationAvoidanceFeelers[0].Extent + CollisionPushOutDistance;
		float const MaxHalfHeight = TargetActor->GetSimpleCollisionHalfHeight() - PushInDistance;
		SafeLocation.Z = FMath::Clamp(ClosestPointOnLineToCapsuleCenter.Z, SafeLocation.Z - MaxHalfHeight, SafeLocation.Z + MaxHalfHeight);

		// 캡슐 표면까지의 거리 계산
		float DistanceSqr;
		TargetRootComponent->GetSquaredDistanceToCollision(ClosestPointOnLineToCapsuleCenter, DistanceSqr, SafeLocation);
		
		// '안전 위치'를 캡슐 안쪽으로 살짝 밀어넣기
		if (PenetrationAvoidanceFeelers.Num() > 0)
		{
			SafeLocation += (SafeLocation - ClosestPointOnLineToCapsuleCenter).GetSafeNormal() * PushInDistance;
		}

		// 벽 뚫림 방지 계산 실행
		bool const bSingleRayPenetrationCheck = !bDoPredictiveAvoidance; // 예측 회피(여러 광선) 사용 여부
		PreventCameraPenetration(*TargetActor, SafeLocation, View.Location, DeltaTime, AimLineToDesiredPosBlockedPct, bSingleRayPenetrationCheck);
	}
}

// 실제 벽 뚫림 방지 계산
void USFCameraMode_ThirdPerson::PreventCameraPenetration(class AActor const& ViewTarget, FVector const& SafeLoc, FVector& CameraLoc, float const& DeltaTime, float& DistBlockedPct, bool bSingleRayOnly)
{
#if ENABLE_DRAW_DEBUG
	DebugActorsHitDuringCameraPenetration.Reset();
#endif

	float HardBlockedPct = DistBlockedPct; // 메인 광선(중앙)이 막힌 비율
	float SoftBlockedPct = DistBlockedPct; // 측면/상하 광선이 막힌 비율

	FVector BaseRay = CameraLoc - SafeLoc; // '안전 위치'에서 '희망 위치'까지의 기본 광선
	FRotationMatrix BaseRayMatrix(BaseRay.Rotation());
	FVector BaseRayLocalUp, BaseRayLocalFwd, BaseRayLocalRight;

	BaseRayMatrix.GetScaledAxes(BaseRayLocalFwd, BaseRayLocalRight, BaseRayLocalUp);

	float DistBlockedPctThisFrame = 1.f; // 이번 프레임에 계산된 '막힘 비율' (1.0 = 안 막힘)

	// 쏠 광선(Ray)의 수 결정 (예측 회피 켜져 있으면 전부, 아니면 1개)
	int32 const NumRaysToShoot = bSingleRayOnly ? FMath::Min(1, PenetrationAvoidanceFeelers.Num()) : PenetrationAvoidanceFeelers.Num();
	
	// 충돌 쿼리 설정 (ViewTarget(캐릭터)은 무시)
	FCollisionQueryParams SphereParams(SCENE_QUERY_STAT(CameraPen), false, nullptr);
	SphereParams.AddIgnoredActor(&ViewTarget);

	FCollisionShape SphereShape = FCollisionShape::MakeSphere(0.f);
	UWorld* World = GetWorld();

	// 설정된 모든 '더듬이(Feeler)' 광선 순회
	for (int32 RayIdx = 0; RayIdx < NumRaysToShoot; ++RayIdx)
	{
		FSFPenetrationAvoidanceFeeler& Feeler = PenetrationAvoidanceFeelers[RayIdx];
		if (Feeler.FramesUntilNextTrace <= 0) // 성능 최적화: 추적 간격(Interval) 확인
		{
			// 1. 광선(Ray) 목표 지점 계산 (기본 광선 + 더듬이 각도)
			FVector RayTarget;
			{
				FVector RotatedRay = BaseRay.RotateAngleAxis(Feeler.AdjustmentRot.Yaw, BaseRayLocalUp);
				RotatedRay = RotatedRay.RotateAngleAxis(Feeler.AdjustmentRot.Pitch, BaseRayLocalRight);
				RayTarget = SafeLoc + RotatedRay;
			}

			SphereShape.Sphere.Radius = Feeler.Extent; // 이 더듬이의 충돌 크기(반경)
			ECollisionChannel TraceChannel = ECC_Camera; // '카메라' 채널과 충돌 검사

			// 2. 광선 발사 (Sweep)
			FHitResult Hit;
			const bool bHit = World->SweepSingleByChannel(Hit, SafeLoc, RayTarget, FQuat::Identity, TraceChannel, SphereShape, SphereParams);
			
#if ENABLE_DRAW_DEBUG
			if (World->TimeSince(LastDrawDebugTime) < 1.f)
			{
				DrawDebugSphere(World, SafeLoc, SphereShape.Sphere.Radius, 8, FColor::Red);
				DrawDebugSphere(World, bHit ? Hit.Location : RayTarget, SphereShape.Sphere.Radius, 8, FColor::Red);
				DrawDebugLine(World, SafeLoc, bHit ? Hit.Location : RayTarget, FColor::Red);
			}
#endif

			// 다음 추적까지의 프레임 간격 초기화
			Feeler.FramesUntilNextTrace = Feeler.TraceInterval;

			const AActor* HitActor = Hit.GetActor();

			// 3. 충돌 감지 시 처리
			if (bHit && HitActor)
			{
				bool bIgnoreHit = false; // 이 충돌을 무시할지 여부

				// 'IgnoreCameraCollision' 태그가 있으면 무시
				if (HitActor->ActorHasTag(SFCameraMode_ThirdPerson_Statics::NAME_IgnoreCameraCollision))
				{
					bIgnoreHit = true;
					SphereParams.AddIgnoredActor(HitActor); // 다음 광선부터 이 액터 무시
				}

				// 'CameraBlockingVolume' 특별 처리
				if (!bIgnoreHit && HitActor->IsA<ACameraBlockingVolume>())
				{
					// 캐릭터가 바라보는 방향(Forward)과 '반대'에 있는 볼륨만 충돌로 인정
					// (캐릭터 '뒤'에 있는 투명 벽만 카메라를 막음)
					const FVector ViewTargetForwardXY = ViewTarget.GetActorForwardVector().GetSafeNormal2D();
					const FVector ViewTargetLocation = ViewTarget.GetActorLocation();
					const FVector HitOffset = Hit.Location - ViewTargetLocation;
					const FVector HitDirectionXY = HitOffset.GetSafeNormal2D();
					const float DotHitDirection = FVector::DotProduct(ViewTargetForwardXY, HitDirectionXY);
					if (DotHitDirection > 0.0f) // 캐릭터 '앞'에 있으면 무시
					{
						bIgnoreHit = true;
						SphereParams.AddIgnoredActor(HitActor);
					}
					else
					{
#if ENABLE_DRAW_DEBUG
						DebugActorsHitDuringCameraPenetration.AddUnique(TObjectPtr<const AActor>(HitActor));
#endif
					}
				}
				
				// 4. 유효한 충돌일 경우 '막힘 비율' 계산
				if (!bIgnoreHit)
				{
					// 충돌한 물체가 폰(Pawn)인지 월드(벽)인지에 따라 다른 가중치 적용
					float const Weight = Cast<APawn>(Hit.GetActor()) ? Feeler.PawnWeight : Feeler.WorldWeight;
					float NewBlockPct = Hit.Time;
					NewBlockPct += (1.f - NewBlockPct) * (1.f - Weight);

					// 'CollisionPushOutDistance'(최소 이격 거리)를 고려하여 최종 막힘 비율 계산
					NewBlockPct = ((Hit.Location - SafeLoc).Size() - CollisionPushOutDistance) / (RayTarget - SafeLoc).Size();
					DistBlockedPctThisFrame = FMath::Min(NewBlockPct, DistBlockedPctThisFrame); // 가장 '많이' 막힌 비율로 갱신

					Feeler.FramesUntilNextTrace = 0; // 충돌했으므로 다음 프레임에도 즉시 검사

#if ENABLE_DRAW_DEBUG
					DebugActorsHitDuringCameraPenetration.AddUnique(TObjectPtr<const AActor>(HitActor));
#endif
				}
			}

			if (RayIdx == 0)
			{
				// 메인 광선(0번)이 막힌 비율 -> HardBlockedPct
				// (엘든링 스타일: 빠른 반응)
				HardBlockedPct = DistBlockedPctThisFrame;
			}
			else
			{
				// 측면/상하 광선이 막힌 비율 -> SoftBlockedPct
				SoftBlockedPct = DistBlockedPctThisFrame;
			}
		}
		else
		{
			// 추적 간격(Interval) 감소
			--Feeler.FramesUntilNextTrace;
		}
	}

	// 5. 최종 '막힘 비율(DistBlockedPct)' 결정 (블렌딩)
	if (bResetInterpolation) // 컷신 등 즉시 이동
	{
		DistBlockedPct = DistBlockedPctThisFrame;
	}
	else if (DistBlockedPct < DistBlockedPctThisFrame)
	{
		// 벽에서 멀어질 때 (막힘 비율 증가): 부드럽게 (PenetrationBlendOutTime)
		if (PenetrationBlendOutTime > DeltaTime)
		{
			DistBlockedPct = DistBlockedPct + DeltaTime / PenetrationBlendOutTime * (DistBlockedPctThisFrame - DistBlockedPct);
		}
		else
		{
			DistBlockedPct = DistBlockedPctThisFrame;
		}
	}
	else
	{
		// 벽에 다가갈 때 (막힘 비율 감소): 빠르게
		if (DistBlockedPct > HardBlockedPct)
		{
			// 메인 광선(Hard)이 막혔으면 즉시 반응
			DistBlockedPct = HardBlockedPct;
		}
		else if (DistBlockedPct > SoftBlockedPct)
		{
			// 측면 광선(Soft)이 막혔으면 설정된 시간(PenetrationBlendInTime)만큼 빠르게 반응
			if (PenetrationBlendInTime > DeltaTime)
			{
				DistBlockedPct = DistBlockedPct - DeltaTime / PenetrationBlendInTime * (DistBlockedPct - SoftBlockedPct);
			}
			else
			{
				DistBlockedPct = SoftBlockedPct;
			}
		}
	}

	// 6. 최종 카메라 위치 적용
	DistBlockedPct = FMath::Clamp<float>(DistBlockedPct, 0.f, 1.f);
	if (DistBlockedPct < (1.f - ZERO_ANIMWEIGHT_THRESH))
	{
		// '안전 위치' + ('희망 방향' * 막힘 비율)
		CameraLoc = SafeLoc + (CameraLoc - SafeLoc) * DistBlockedPct;
	}
}

// '앉기' 목표 오프셋(위치 보정) 설정
void USFCameraMode_ThirdPerson::SetTargetCrouchOffset(FVector NewTargetOffset)
{
	CrouchOffsetBlendPct = 0.0f;				// 블렌드 진행도 초기화
	InitialCrouchOffset = CurrentCrouchOffset;	// 현재 위치를 시작점으로
	TargetCrouchOffset = NewTargetOffset;		// 새 위치를 목표점으로
}

// '앉기' 오프셋 부드럽게 보간
void USFCameraMode_ThirdPerson::UpdateCrouchOffset(float DeltaTime)
{
	if (CrouchOffsetBlendPct < 1.0f)
	{
		CrouchOffsetBlendPct = FMath::Min(CrouchOffsetBlendPct + DeltaTime * CrouchOffsetBlendMultiplier, 1.0f);
		CurrentCrouchOffset = FMath::InterpEaseInOut(InitialCrouchOffset, TargetCrouchOffset, CrouchOffsetBlendPct, 1.0f);
	}
	else
	{
		CurrentCrouchOffset = TargetCrouchOffset;
		CrouchOffsetBlendPct = 1.0f;
	}
}

// '점프' 목표 오프셋 설정
void USFCameraMode_ThirdPerson::SetTargetJumpOffset(FVector NewTargetOffset)
{
	JumpOffsetBlendPct = 0.f;
	InitialJumpOffset = CurrentJumpOffset;
	TargetJumpOffset = NewTargetOffset;
}

// '점프' 오프셋 부드럽게 보간
void USFCameraMode_ThirdPerson::UpdateJumpOffset(float DeltaTime)
{
	if (JumpOffsetBlendPct < 1.f)
	{
		JumpOffsetBlendPct = FMath::Min(JumpOffsetBlendPct + DeltaTime * JumpOffsetBlendMultiplier, 1.f);
		CurrentJumpOffset = FMath::InterpEaseInOut(InitialJumpOffset, TargetJumpOffset, JumpOffsetBlendPct, 1.f);
	}
	else
	{
		CurrentJumpOffset = TargetJumpOffset;
		JumpOffsetBlendPct = 1.f;
	}
}

