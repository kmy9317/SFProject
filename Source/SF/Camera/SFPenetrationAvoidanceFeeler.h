// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "SFPenetrationAvoidanceFeeler.generated.h"

/**
 * Struct defining a feeler ray used for camera penetration avoidance.
 * 카메라의 벽 뚫림(관통) 방지를 위한 '더듬이(Feeler)' 광선(Ray) 정의 구조체.
 */
USTRUCT()
struct FSFPenetrationAvoidanceFeeler
{
	GENERATED_BODY()

	/** 메인 광선(중심)으로부터의 각도 편차 */
	UPROPERTY(EditAnywhere, Category=PenetrationAvoidanceFeeler)
	FRotator AdjustmentRot;

	/** 이 더듬이가 '월드(벽 등)'에 부딪혔을 때, 최종 카메라 위치에 영향을 줄 가중치 */
	UPROPERTY(EditAnywhere, Category=PenetrationAvoidanceFeeler)
	float WorldWeight;

	/** 이 더듬이가 'Pawn(캐릭터 등)'에 부딪혔을 때, 최종 카메라 위치에 영향을 줄 가중치 (0이면 Pawn 무시) */
	UPROPERTY(EditAnywhere, Category=PenetrationAvoidanceFeeler)
	float PawnWeight;

	/** 이 더듬이의 충돌 감지 범위(크기) */
	UPROPERTY(EditAnywhere, Category=PenetrationAvoidanceFeeler)
	float Extent;

	/** 지난 프레임에 아무것도 감지되지 않았을 때, 다음 감지까지의 최소 프레임 간격 (성능 최적화용) */
	UPROPERTY(EditAnywhere, Category=PenetrationAvoidanceFeeler)
	int32 TraceInterval;

	/** 이 더듬이가 마지막으로 사용된 후 다음 감지까지 남은 프레임 수 */
	UPROPERTY(transient)
	int32 FramesUntilNextTrace;


	FSFPenetrationAvoidanceFeeler()
		: AdjustmentRot(ForceInit)
		, WorldWeight(0)
		, PawnWeight(0)
		, Extent(0)
		, TraceInterval(0)
		, FramesUntilNextTrace(0)
	{
	}

	FSFPenetrationAvoidanceFeeler(const FRotator& InAdjustmentRot,
									const float& InWorldWeight, 
									const float& InPawnWeight, 
									const float& InExtent, 
									const int32& InTraceInterval = 0, 
									const int32& InFramesUntilNextTrace = 0)
		: AdjustmentRot(InAdjustmentRot)
		, WorldWeight(InWorldWeight)
		, PawnWeight(InPawnWeight)
		, Extent(InExtent)
		, TraceInterval(InTraceInterval)
		, FramesUntilNextTrace(InFramesUntilNextTrace)
	{
	}
};

