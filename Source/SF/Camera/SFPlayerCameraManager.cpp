// Copyright Epic Games, Inc. All Rights Reserved.

#include "SFPlayerCameraManager.h"
#include "SFCameraMode.h"

#include "Async/TaskGraphInterfaces.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "SFCameraComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SFPlayerCameraManager)

class FDebugDisplayInfo;

ASFPlayerCameraManager::ASFPlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 기본 시야각(FOV) 및 상하 각도 제한 설정
	DefaultFOV = SF_CAMERA_DEFAULT_FOV;
	ViewPitchMin = SF_CAMERA_DEFAULT_PITCH_MIN;
	ViewPitchMax = SF_CAMERA_DEFAULT_PITCH_MAX;
}

void ASFPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	Super::UpdateViewTarget(OutVT, DeltaTime);
}

void ASFPlayerCameraManager::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	check(Canvas);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	// 디버그 정보 표시 설정
	DisplayDebugManager.SetFont(GEngine->GetSmallFont());
	DisplayDebugManager.SetDrawColor(FColor::Yellow);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("SFPlayerCameraManager: %s"), *GetNameSafe(this)));

	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	const APawn* Pawn = (PCOwner ? PCOwner->GetPawn() : nullptr);

	// 폰(캐릭터)에 부착된 SFCameraComponent 찾기
	if (const USFCameraComponent* CameraComponent = USFCameraComponent::FindCameraComponent(Pawn))
	{
		// 카메라 컴포넌트의 디버그 정보도 함께 표시
		CameraComponent->DrawDebug(Canvas);
	}
}

