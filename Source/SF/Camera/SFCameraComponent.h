// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"

#include "SFCameraComponent.generated.h"

class UCanvas;
class USFCameraMode;
class USFCameraModeStack;
class UObject;
struct FFrame;
struct FGameplayTag;
struct FMinimalViewInfo;
template <class TClass> class TSubclassOf;

// 카메라 모드를 결정하는 델리게이트(함수 포인터) 선언.
DECLARE_DYNAMIC_DELEGATE_RetVal(TSubclassOf<USFCameraMode>, FSFCameraModeDelegate);

/**
 * USFCameraComponent
 *
 * 【카메라 컴포넌트】
 * 캐릭터에 부착하여 사용하는 실제 카메라.
 * * 주요 기능:
 * - 카메라 모드(CameraMode) 관리 및 블렌딩
 * - 매 프레임 카메라 위치/회전/FOV 계산
 * - 플레이어 컨트롤러의 시점 회전과 동기화
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class SF_API USFCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

public:

	USFCameraComponent(const FObjectInitializer& ObjectInitializer);

	// 액터(캐릭터 등)에서 이 카메라 컴포넌트를 찾아 반환.
	UFUNCTION(BlueprintPure, Category = "SF|Camera")
	static USFCameraComponent* FindCameraComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<USFCameraComponent>() : nullptr); }

	// 카메라가 바라보는 대상 액터(보통 이 컴포넌트를 소유한 캐릭터) 반환.
	virtual AActor* GetTargetActor() const { return GetOwner(); }

	// 어떤 카메라 모드를 사용할지 결정하는 함수를 연결하는 변수.
	// (예: "기본 3인칭 모드" 또는 "조준 모드"를 반환하는 함수 연결)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SF|Camera")
	FSFCameraModeDelegate DetermineCameraModeDelegate;

	// FOV(시야각)에 일시적인 오프셋(추가 값) 추가 (1프레임만 유효).
	void AddFieldOfViewOffset(float FovOffset) { FieldOfViewOffset += FovOffset; }

	// 디버그(개발용) 정보 화면 표시.
	virtual void DrawDebug(UCanvas* Canvas) const;

	// 현재 활성화된 카메라 모드(스택 최상단)의 블렌드 정보 획득.
	void GetBlendInfo(float& OutWeightOfTopLayer, FGameplayTag& OutTagOfTopLayer) const;

protected:

	virtual void OnRegister() override;
	// 실제 카메라 뷰(시점) 계산.
	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;

	// 카메라 모드 스택 업데이트 (DetermineCameraModeDelegate 실행 등).
	virtual void UpdateCameraModes();

protected:

	// 여러 카메라 모드를 블렌딩(혼합)하는 스택(관리자).
	// (예: 일반 모드 → 조준 모드로 부드럽게 전환)
	UPROPERTY()
	TObjectPtr<USFCameraModeStack> CameraModeStack;

	// FOV(시야각) 오프셋 (1프레임 후 자동으로 0으로 초기화됨).
	float FieldOfViewOffset;
};