// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Camera/PlayerCameraManager.h"

#include "SFPlayerCameraManager.generated.h"

class FDebugDisplayInfo;
class UCanvas;
class UObject;

/**
 * ASFPlayerCameraManager
 *
 *	SF 프로젝트용 카메라 매니저
 */
UCLASS(notplaceable, MinimalAPI)
class ASFPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:

	ASFPlayerCameraManager(const FObjectInitializer& ObjectInitializer);

protected:

	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;
};

