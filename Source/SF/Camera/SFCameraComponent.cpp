// Copyright Epic Games, Inc. All Rights Reserved.

#include "SFCameraComponent.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "SFCameraMode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFCameraComponent)


USFCameraComponent::USFCameraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CameraModeStack = nullptr;
	FieldOfViewOffset = 0.0f;
}

// 컴포넌트 등록 시 호출
void USFCameraComponent::OnRegister()
{
	Super::OnRegister();

	// 카메라 모드 스택(관리자)이 없으면 새로 생성
	if (!CameraModeStack)
	{
		CameraModeStack = NewObject<USFCameraModeStack>(this);
		check(CameraModeStack);
	}
}

// 이 카메라 컴포넌트의 뷰(시점) 계산 (매 프레임 호출됨)
void USFCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	check(CameraModeStack);

	// 현재 어떤 카메라 모드를 사용할지 결정 (예: 3인칭 or 조준)
	UpdateCameraModes();

	// 스택이 비어있으면 아무것도 하지 않음
	if (CameraModeStack->IsStackEmpty())
	{
		return;
	}

	// 카메라 모드 스택을 평가(계산)하여 최종 '뷰' 결과물(CameraModeView) 획득
	FSFCameraModeView CameraModeView;
	CameraModeStack->EvaluateStack(DeltaTime, CameraModeView);

	// 플레이어 컨트롤러가 최신 뷰(시점)를 따라가도록 동기화
	// (이걸 해줘야 캐릭터가 카메라가 보는 방향으로 움직임)
	if (APawn* TargetPawn = Cast<APawn>(GetTargetActor()))
	{
		if (APlayerController* PC = TargetPawn->GetController<APlayerController>())
		{
			PC->SetControlRotation(CameraModeView.ControlRotation);
		}
	}

	// 시야각(FOV)에 추가된 오프셋(임시 값, 예: 1프레임 줌인) 적용
	CameraModeView.FieldOfView += FieldOfViewOffset;
	FieldOfViewOffset = 0.0f; // 오프셋은 1프레임만 유효 (즉시 초기화)

	// 카메라 컴포넌트(자신)가 최신 뷰를 따라가도록 위치/회전/FOV 동기화
	SetWorldLocationAndRotation(CameraModeView.Location, CameraModeView.Rotation);
	FieldOfView = CameraModeView.FieldOfView;

	// 최종 뷰 정보(DesiredView) 채우기
	DesiredView.Location = CameraModeView.Location;
	DesiredView.Rotation = CameraModeView.Rotation;
	DesiredView.FOV = CameraModeView.FieldOfView;
	DesiredView.OrthoWidth = OrthoWidth;
	DesiredView.OrthoNearClipPlane = OrthoNearClipPlane;
	DesiredView.OrthoFarClipPlane = OrthoFarClipPlane;
	DesiredView.AspectRatio = AspectRatio;
	DesiredView.bConstrainAspectRatio = bConstrainAspectRatio;
	DesiredView.bUseFieldOfViewForLOD = bUseFieldOfViewForLOD;
	DesiredView.ProjectionMode = ProjectionMode;

	DesiredView.PostProcessBlendWeight = PostProcessBlendWeight;
	if (PostProcessBlendWeight > 0.0f)
	{
		DesiredView.PostProcessSettings = PostProcessSettings;
	}


	if (IsXRHeadTrackedCamera())
	{
		// XR(VR/AR) 환경에서는 위 카메라 로직 대부분이 무관하지만,
		// 포스트프로세스 설정 등은 유효하므로 부모 클래스 로직 호출
		Super::GetCameraView(DeltaTime, DesiredView);
	}
}

// 현재 상황에 맞는 카메라 모드 결정 및 스택에 추가(Push)
void USFCameraComponent::UpdateCameraModes()
{
	check(CameraModeStack);

	if (CameraModeStack->IsStackActivate())
	{
		// 'DetermineCameraModeDelegate'(델리게이트)에 함수가 연결되어 있는지 확인
		if (DetermineCameraModeDelegate.IsBound())
		{
			// 델리게이트 실행 (예: '조준 중인가?' -> '조준 모드' 반환)
			if (const TSubclassOf<USFCameraMode> CameraMode = DetermineCameraModeDelegate.Execute())
			{
				// 반환된 카메라 모드를 스택의 최상단에 추가(Push)
				CameraModeStack->PushCameraMode(CameraMode);
			}
		}
	}
}

void USFCameraComponent::DisableYawLimitsForMode(TSubclassOf<USFCameraMode> CameraModeClass)
{
	if (CameraModeStack)
	{
		CameraModeStack->DisableYawLimitsForMode(CameraModeClass);
	}
}

void USFCameraComponent::DisableAllYawLimitsTemporarily()
{
	if (CameraModeStack)
	{
		CameraModeStack->DisableAllYawLimitsTemporarily();
	}
}

void USFCameraComponent::DrawDebug(UCanvas* Canvas) const
{
	check(Canvas);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	DisplayDebugManager.SetFont(GEngine->GetSmallFont());
	DisplayDebugManager.SetDrawColor(FColor::Yellow);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("SFCameraComponent: %s"), *GetNameSafe(GetTargetActor())));

	DisplayDebugManager.SetDrawColor(FColor::White);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("   Location: %s"), *GetComponentLocation().ToCompactString()));
	DisplayDebugManager.DrawString(FString::Printf(TEXT("   Rotation: %s"), *GetComponentRotation().ToCompactString()));
	DisplayDebugManager.DrawString(FString::Printf(TEXT("   FOV: %f"), FieldOfView));

	check(CameraModeStack);
	CameraModeStack->DrawDebug(Canvas);
}

// 스택 최상단(혹은 최하단) 모드의 블렌드 정보 반환
void USFCameraComponent::GetBlendInfo(float& OutWeightOfTopLayer, FGameplayTag& OutTagOfTopLayer) const
{
	check(CameraModeStack);
	CameraModeStack->GetBlendInfo(/*out*/ OutWeightOfTopLayer, /*out*/ OutTagOfTopLayer);
}
