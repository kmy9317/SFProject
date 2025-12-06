// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "Templates/SubclassOf.h"
#include "SFHeroComponent.generated.h"

struct FInputActionValue;
class USFCameraMode;
/**
 * component that sets up input and camera handling for player controlled pawns (or bots that simulate players)
 * - this depends on a PawnExtensionComponent to coordinate initialization
 *
 * 카메라, 입력 등 플레이어가 제어하는 시스템의 초기화를 처리하는 컴포넌트
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFHeroComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:

	USFHeroComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "LC|Hero")
	static USFHeroComponent* FindHeroComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<USFHeroComponent>() : nullptr); }
	
	/** Overrides the camera from an active gameplay ability */
	//void SetAbilityCameraMode(TSubclassOf<ULCCameraMode> CameraMode, const FGameplayAbilitySpecHandle& OwningSpecHandle);
	
	/** Clears the camera override if it is set */
	//void ClearAbilityCameraMode(const FGameplayAbilitySpecHandle& OwningSpecHandle);
	
	/** The name of the extension event sent via UGameFrameworkComponentManager when ability inputs are ready to bind */
	static const FName NAME_BindInputsNow;
	
	/** FeatureName 정의 */
	static const FName NAME_ActorFeatureName;
	
	//~ Begin IGameFrameworkInitStateInterface interface
	 virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	 virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	 virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	 virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	 virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface interface

	// 마지막 입력 방향(월드 공간)을 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "SF|Input")
	FVector GetLastInputDirection() const { return LastInputDirection; }

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void InitializePlayerInput(UInputComponent* PlayerInputComponent);
	void Input_AbilityInputTagStarted(FGameplayTag InputTag);
	void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);
	
	void Input_Move(const FInputActionValue& InputActionValue);
	void Input_LookMouse(const FInputActionValue& InputActionValue);
	void Input_Crouch(const FInputActionValue& InputActionValue);

	void InitializeHUD();

	UFUNCTION()
	TSubclassOf<USFCameraMode> DetermineCameraMode();

protected:

	// 입력 방향 의도 저장용 변수 추가
	UPROPERTY(Transient)
	FVector LastInputDirection;
	
	/** Camera mode set by an ability. */
	// UPROPERTY()
	// TSubclassOf<ULCCameraMode> AbilityCameraMode;
	
	/** Spec handle for the last ability to set a camera mode. */
	//FGameplayAbilitySpecHandle AbilityCameraModeOwningSpecHandle;
};
