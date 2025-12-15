// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/World.h"
#include "GameplayTagContainer.h"

#include "SFCameraMode.generated.h"

class AActor;
class UCanvas;
class USFCameraComponent;

// 카메라 기본 설정
#define SF_CAMERA_DEFAULT_FOV			(+75.0f)    // 기본 시야각
#define SF_CAMERA_DEFAULT_PITCH_MIN		(-70.0f)    // 카메라 최소 상하 각도
#define SF_CAMERA_DEFAULT_PITCH_MAX		(+60.0f)    // 카메라 최대 상하 각도

/**
 * ESFCameraModeBlendFunction
 *
 *	카메라 모드 간 전환 시 사용할 블렌드(혼합) 함수 종류.
 */
UENUM(BlueprintType)
enum class ESFCameraModeBlendFunction : uint8
{
	// 단순 선형 보간 (일정한 속도로 변경).
	Linear,

	// 즉시 가속 후 부드럽게 감속하여 목표 도달.
	EaseIn,

	// 부드럽게 가속 후 감속 없이 목표 도달.
	EaseOut,

	// 부드럽게 가속 및 감속.
	EaseInOut,

	COUNT	UMETA(Hidden)
};


/**
 * FSFCameraModeView
 *
 *	카메라 모드 블렌딩(혼합)에 사용되는 뷰 데이터(위치, 회전, 시야각 등).
 */
struct FSFCameraModeView
{
public:
	FSFCameraModeView();

	void Blend(const FSFCameraModeView& Other, float OtherWeight);

public:
	FVector Location;         // 카메라 위치
	FRotator Rotation;        // 카메라 회전
	FRotator ControlRotation; // 플레이어 조작 회전
	float FieldOfView;        // 시야각
};


/**
 * USFCameraMode
 *
 *	모든 카메라 모드의 기본(부모) 클래스.
 */
UCLASS(Abstract, NotBlueprintable)
class SF_API USFCameraMode : public UObject
{
	GENERATED_BODY()

public:

	USFCameraMode();

	USFCameraComponent* GetSFCameraComponent() const;

	virtual UWorld* GetWorld() const override;

	AActor* GetTargetActor() const;

	const FSFCameraModeView& GetCameraModeView() const { return View; }

	virtual void OnActivation();

	virtual void OnDeactivation() {};
	
	void UpdateCameraMode(float DeltaTime);

	float GetBlendTime() const { return BlendTime; }
	float GetBlendWeight() const { return BlendWeight; }
	void SetBlendWeight(float Weight);
	
	void SetYawLimitsEnabled(bool bEnabled) { bUseYawLimits = bEnabled; }
	void SetYawLimitsTemporarilyDisabled(bool bDisabled) { bYawLimitsTemporarilyDisabled = bDisabled; }
	bool IsYawLimitsActive() const { return bUseYawLimits && !bYawLimitsTemporarilyDisabled; }

	FGameplayTag GetCameraTypeTag() const
	{
		return CameraTypeTag;
	}

	virtual void DrawDebug(UCanvas* Canvas) const;

protected:
	// 피벗(중심축) 위치 계산 (보통 캐릭터의 머리나 소켓 위치).
	virtual FVector GetPivotLocation() const;
	// 피벗 회전값 계산 (보통 플레이어의 조작 방향).
	virtual FRotator GetPivotRotation() const;

	// 매 프레임 뷰(시점) 업데이트.
	virtual void UpdateView(float DeltaTime);
	// 블렌딩(전환) 상태 업데이트.
	virtual void UpdateBlending(float DeltaTime);

protected:
	// 이 카메라 모드의 종류를 식별하는 태그.
	// (예: "조준" 태그가 활성화되면 정확도 향상 로직 실행 등)
	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	FGameplayTag CameraTypeTag;
	
	// 카메라의 중심(피벗)이 될 캐릭터 메쉬의 소켓 이름.
	UPROPERTY(EditDefaultsOnly)
	FName CameraSocketName;
	
	// 이 카메라 모드가 생성한 뷰(시점) 결과물.
	FSFCameraModeView View;

	// 수평 시야각 (단위: 도).
	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "5.0", UIMax = "170", ClampMin = "5.0", ClampMax = "170.0"))
	float FieldOfView;

	// 최소 뷰 피치 (카메라 상하 각도 최솟값).
	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89.9", ClampMax = "89.9"))
	float ViewPitchMin;

	// 최대 뷰 피치 (카메라 상하 각도 최댓값).
	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89.9", ClampMax = "89.9"))
	float ViewPitchMax;

	// Yaw 제한 사용 여부
	UPROPERTY(EditDefaultsOnly, Category = "View")
	bool bUseYawLimits = false;

	// 캐릭터 기준 좌우 최대 각도 (예: -90 ~ +90)
	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (EditCondition = "bUseYawLimits"))
	float ViewYawMin = -90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (EditCondition = "bUseYawLimits"))
	float ViewYawMax = 90.0f;
    
	// Yaw 보간 속도 (높을수록 빠르게 제한 범위로 이동)
	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (EditCondition = "bUseYawLimits"))
	float YawLimitInterpSpeed = 5.0f;

	// Yaw 제한 보간용
	float CurrentYawOffset = 0.0f;

	// Yaw 보간 중인지 추적
	bool bIsYawInterpolating = false;

	float PreviousRelativeYaw = 0.0f;

	// 런타임 임시 비활성화 플래그
	bool bYawLimitsTemporarilyDisabled = false;

	// 이 모드로 완전히 블렌드(전환)되는 데 걸리는 시간.
	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	float BlendTime;

	// 블렌딩(전환)에 사용할 함수 (위의 ESFCameraModeBlendFunction 참고).
	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	ESFCameraModeBlendFunction BlendFunction;

	// 블렌드 함수의 곡선 형태를 조절하는 지수 값 (숫자가 클수록 변화가 급격함).
	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	float BlendExponent;

	// 0.0 ~ 1.0 사이의 선형 블렌드 진행도.
	float BlendAlpha;

	// 실제 블렌드 함수가 적용된 최종 가중치 (0.0 ~ 1.0).
	float BlendWeight;

protected:
	/** true면 모든 보간(부드러운 이동)을 건너뛰고 즉시 이상적인 위치로 카메라 이동. 다음 프레임에 자동으로 false가 됨. */
	UPROPERTY(transient)
	uint32 bResetInterpolation:1;
};


/**
 * USFCameraModeStack
 *
 *	여러 카메라 모드를 쌓고 블렌딩(혼합)하기 위한 스택(관리자).
 * (예: '기본 3인칭 모드' 위에 '조준 모드'를 쌓고 부드럽게 전환)
 */
UCLASS()
class USFCameraModeStack : public UObject
{
	GENERATED_BODY()

public:
	USFCameraModeStack();

	void ActivateStack();
	void DeactivateStack();

	bool IsStackActivate() const { return bIsActive; }

	bool IsStackEmpty() const { return CameraModeStack.Num() == 0; }

	// 새로운 카메라 모드를 스택의 맨 위에 추가 (푸시).
	void PushCameraMode(TSubclassOf<USFCameraMode> CameraModeClass);

	// 스택을 평가(계산)하여 최종 카메라 뷰 결과물 반환.
	bool EvaluateStack(float DeltaTime, FSFCameraModeView& OutCameraModeView);

	void DrawDebug(UCanvas* Canvas) const;

	// 가장 위쪽 레이어(현재 가장 활성화된 모드)의 블렌드 가중치와 태그 정보 획득.
	void GetBlendInfo(float& OutWeightOfTopLayer, FGameplayTag& OutTagOfTopLayer) const;

	void DisableYawLimitsForMode(TSubclassOf<USFCameraMode> CameraModeClass);

	void DisableAllYawLimitsTemporarily();

protected:

	// 카메라 모드 클래스(설계도)로부터 실제 인스턴스(객체)를 가져오거나 생성.
	USFCameraMode* GetCameraModeInstance(TSubclassOf<USFCameraMode> CameraModeClass);

	// 매 프레임 스택 상태 업데이트.
	void UpdateStack(float DeltaTime);
	// 스택에 쌓인 모드들을 아래에서부터 위로 블렌딩하여 최종 뷰 생성.
	void BlendStack(FSFCameraModeView& OutCameraModeView) const;
	
protected:

	// 이 스택이 활성화되어 있는지 여부.
	bool bIsActive;

	// 생성된 모든 카메라 모드 인스턴스(객체) 목록 (재사용 목적).
	UPROPERTY()
	TArray<TObjectPtr<USFCameraMode>> CameraModeInstances;

	// 현재 활성화되어 블렌딩 중인 카메라 모드 스택. (0번이 가장 위 = 가장 영향력 높음)
	UPROPERTY()
	TArray<TObjectPtr<USFCameraMode>> CameraModeStack;
};

