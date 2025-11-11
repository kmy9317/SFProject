// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SFCameraMode.h"
#include "Curves/CurveFloat.h"
#include "SFPenetrationAvoidanceFeeler.h"
#include "DrawDebugHelpers.h"
#include "SFCameraMode_ThirdPerson.generated.h"

class UCurveVector;

/**
 * USFCameraMode_ThirdPerson
 *
 *	3인칭 카메라 모드
 */
UCLASS(Abstract, Blueprintable)
class USFCameraMode_ThirdPerson : public USFCameraMode
{
	GENERATED_BODY()

public:
	USFCameraMode_ThirdPerson();

protected:
	// 매 프레임 뷰(시점) 업데이트 (3인칭 로직 적용).
	virtual void UpdateView(float DeltaTime) override;

	// 타겟(캐릭터)의 상태(앉기, 점프 등)에 맞춰 뷰 업데이트.
	void UpdateForTarget(float DeltaTime);
	// 벽 뚫림(관통) 방지 로직 업데이트.
	void UpdatePreventPenetration(float DeltaTime);
	// 실제 벽 뚫림 방지 계산 수행.
	void PreventCameraPenetration(class AActor const& ViewTarget, FVector const& SafeLoc, FVector& CameraLoc, float const& DeltaTime, float& DistBlockedPct, bool bSingleRayOnly);
	
	virtual void DrawDebug(UCanvas* Canvas) const override;

protected:
	// Pitch에 따라 카메라 거리 조절하는 커브
	// - 위를 볼 때: 더 멀리 (적 확인 용이)
	// - 아래를 볼 때: 가까이 (발밑 확인 용이)
	UPROPERTY(EditDefaultsOnly, Category = "Third Person", Meta = (EditCondition = "!bUseRuntimeFloatCurves"))
	TObjectPtr<const UCurveVector> TargetOffsetCurve;

	// (주석 생략 - TargetOffsetCurve와 유사한 회전용 커브)
	UPROPERTY(EditDefaultsOnly, Category = "Third Person")
	TObjectPtr<const UCurveVector> TargetRotationCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Third Person")
	bool bUseRuntimeFloatCurves;

	UPROPERTY(EditDefaultsOnly, Category = "Third Person", Meta = (EditCondition = "bUseRuntimeFloatCurves"))
	FRuntimeFloatCurve TargetOffsetX;
	UPROPERTY(EditDefaultsOnly, Category = "Third Person", Meta = (EditCondition = "bUseRuntimeFloatCurves"))
	FRuntimeFloatCurve TargetOffsetY;
	UPROPERTY(EditDefaultsOnly, Category = "Third Person", Meta = (EditCondition = "bUseRuntimeFloatCurves"))
	FRuntimeFloatCurve TargetOffsetZ;

	// 앉기 상태일 때 카메라 오프셋(위치 보정) 블렌드 속도.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person")
	float CrouchOffsetBlendMultiplier = 5.0f;
	
	// 점프 상태일 때 카메라 오프셋 블렌드 속도.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person")
	float JumpOffsetBlendMultiplier = 5.0f;

	// ============= 벽 뚫림 방지 설정 =============
public:
	// 벽에 닿았을 때 블렌드 인(카메라 당기기) 시간 (빠른 반응).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Collision")
	float PenetrationBlendInTime = 0.05f;

	// 벽에서 멀어질 때 블렌드 아웃(카메라 풀기) 시간 (부드러운 복귀).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Collision")
	float PenetrationBlendOutTime = 0.2f;

	/** 벽 뚫림 방지 기능 활성화 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Collision")
	bool bPreventPenetration = true;

	/** 예측 회피 기능 활성화 (벽이 다가올 것을 예상해 미리 카메라 당김) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Collision")
	bool bDoPredictiveAvoidance = true;

	// 충돌 시 카메라를 벽으로부터 밀어내는 최소 거리.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	float CollisionPushOutDistance = 5.f;

	/** 카메라가 얼마나 막혔는지(가려졌는지) 리포트하는 비율 값 (0.0 ~ 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	float ReportPenetrationPercent = 0.f;

	/**
	 * 벽 뚫림 방지용 '더듬이(Feeler)' 배열
	 * (SFPenetrationAvoidanceFeeler.h 참고)
	 * Index 0: 메인 광선 (중앙, 가장 중요)
	 * Index 1+: 예측 광선들 (좌우상하)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Collision")
	TArray<FSFPenetrationAvoidanceFeeler> PenetrationAvoidanceFeelers;

	UPROPERTY(Transient)
	float AimLineToDesiredPosBlockedPct;

	UPROPERTY(Transient)
	TArray<TObjectPtr<const AActor>> DebugActorsHitDuringCameraPenetration;

#if ENABLE_DRAW_DEBUG
	mutable float LastDrawDebugTime = -MAX_FLT;
#endif

protected:
	void SetTargetCrouchOffset(FVector NewTargetOffset);
	void UpdateCrouchOffset(float DeltaTime);

	FVector InitialCrouchOffset = FVector::ZeroVector;
	FVector TargetCrouchOffset = FVector::ZeroVector;
	float CrouchOffsetBlendPct = 1.0f;
	FVector CurrentCrouchOffset = FVector::ZeroVector;

protected:
	void SetTargetJumpOffset(FVector NewTargetOffset);
	void UpdateJumpOffset(float DeltaTime);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FVector JumpOffset = FVector::ZeroVector;
	
	FVector InitialJumpOffset = FVector::ZeroVector;
	FVector TargetJumpOffset = FVector::ZeroVector;
	float JumpOffsetBlendPct = 1.0f;
	FVector CurrentJumpOffset = FVector::ZeroVector;
};

