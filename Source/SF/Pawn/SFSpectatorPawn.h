// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"
#include "SFSpectatorPawn.generated.h"

class USFCameraComponent;
class USFCameraMode;

UCLASS()
class SF_API ASFSpectatorPawn : public ASpectatorPawn
{
	GENERATED_BODY()

public:
	ASFSpectatorPawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual FVector GetPawnViewLocation() const override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void SetFollowTarget(AActor* InTarget);
	AActor* GetFollowTarget() const { return FollowTarget.Get(); }

protected:
	UFUNCTION()
	TSubclassOf<USFCameraMode> DetermineCameraMode();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Camera")
	TObjectPtr<USFCameraComponent> CameraComponent;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Camera")
	TSubclassOf<USFCameraMode> DefaultCameraModeClass;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> FollowTarget;

	/** 스무딩된 타겟 위치 */
	FVector SmoothedTargetLocation;
    
	/** 스무딩된 타겟 회전 */
	FRotator SmoothedTargetRotation;

	/** 타겟 위치 스무딩 속도 (네트워크 지터 흡수) */
	UPROPERTY(EditDefaultsOnly, Category = "SF|Spectator")
	float TargetSmoothingSpeed = 10.f;

	/** SpectatorPawn 위치 따라가기 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "SF|Spectator")
	float LocationFollowSpeed = 15.f;

	/** 회전 따라가기 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "SF|Spectator")
	float RotationFollowSpeed = 10.f;
};
