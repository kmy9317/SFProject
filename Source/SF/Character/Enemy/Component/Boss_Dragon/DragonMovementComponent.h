// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "DragonMovementComponent.generated.h"


class UCharacterMovementComponent;
class USFDragonMovementStateBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFOnStateChanged, FGameplayTag, OldState, FGameplayTag, NewState);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SF_API USFDragonMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USFDragonMovementComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void InitializeDragonMovementComponent();
	

#pragma region State Management
protected:
	void InitializeStateManagement();
	void UpdateGroundDistance();

	// === Friend 선언 (State 클래스들이 SetMovementState 접근 가능) ===
	friend class USFDragonMovementStateBase;
	friend class USFDragonGroundedState;
	friend class USFDragonTakingOffState;
	friend class USFDragonFlyingState;
	friend class USFDragonHoveringState;
	friend class USFDragonDivingState;
	friend class USFDragonGlidingState;
	friend class USFDragonLandingState;
	friend class USFDragonDisabledState;

public:
	//  Helper 함수들 

	// 비행 시작 (Grounded → TakingOff → Flying)
	UFUNCTION(BlueprintCallable, Category = "Dragon Movement")
	void StartFlying();

	// 착지 시작 (Flying/Hovering → Landing → Grounded)
	UFUNCTION(BlueprintCallable, Category = "Dragon Movement")
	void StartLanding();

	// 급강하 시작 (Flying/Hovering → Diving → Hovering)
	UFUNCTION(BlueprintCallable, Category = "Dragon Movement")
	void StartDiving();

	// 활공 시작 (Flying/Hovering → Gliding)
	UFUNCTION(BlueprintCallable, Category = "Dragon Movement")
	void StartGliding();

	// 호버링 시작 (Flying → Hovering)
	UFUNCTION(BlueprintCallable, Category = "Dragon Movement")
	void StartHovering();

	UFUNCTION(BlueprintCallable,BlueprintPure)
	FGameplayTag GetCurrentStateTag() const { return CurrentStateTag; }
	

private:
	// 직접 State 전환 
	void SetMovementState(FGameplayTag NewStateTag);

protected:
	void StateChanged(FGameplayTag OldStateTag, FGameplayTag NewStateTag);
#pragma endregion

#pragma region Getters - State
public:
	FORCEINLINE USFDragonMovementStateBase* GetCurrentState() const { return CurrentState; }
	FORCEINLINE UCharacterMovementComponent* GetCharacterMovementComponent() const { return CachedCharacterMovementComponent; }
#pragma endregion

#pragma region Getters - Flight
	FORCEINLINE float GetCurrentFlightSpeed() const { return CurrentFlightSpeed; }
	FORCEINLINE float GetMinimumFlightHeight() const { return MinimumFlightHeight; }
	FORCEINLINE float GetFlightRotationSpeed() const { return FlightRotationSpeed; }
	FORCEINLINE FVector GetTargetOffset() const { return TargetOffset; }
	FORCEINLINE float GetDiveSpeed() const { return DiveSpeed; }
	FORCEINLINE float GetDiveGravity() const { return DiveGravity; }
	FORCEINLINE float GetDivingRotationSpeed() const { return DivingRotationSpeed; }
	FORCEINLINE float GetHoveringDistance() const { return HoveringDistance; }
	FORCEINLINE float GetHoveringSpeed() const { return HoveringSpeed; }
#pragma endregion

#pragma region Getters - Takeoff & Landing 
	FORCEINLINE float GetTakeOffSpeed() const { return TakeOffSpeed; }
	FORCEINLINE FVector GetTakeOffDirection() const { return TakeOffDirection; }
	FORCEINLINE float GetLandingSpeed() const { return LandingSpeed; }
	FORCEINLINE float GetLandingRotationSpeed() const { return LandingRotationSpeed; }
	FORCEINLINE FVector GetTargetLandingLocation() const { return TargetLandingLocation; }
	FORCEINLINE float GetGroundDistance() const { return CurrentGroundDistance; }
#pragma endregion

#pragma region Getters - Target
	FORCEINLINE AActor* GetTargetActor() const { return TargetActor; }
#pragma endregion

#pragma region State Properties
protected:
	UPROPERTY()
	USFDragonMovementStateBase* CurrentState;

	UPROPERTY()
	TMap<FGameplayTag, USFDragonMovementStateBase*> States;

	UPROPERTY()
	FGameplayTag CurrentStateTag;
#pragma endregion

#pragma region Flight Properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float CurrentFlightSpeed = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float MinimumFlightHeight = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float FlightRotationSpeed = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	FVector TargetOffset = FVector(0.0f, 0.0f, 500.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float DiveSpeed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float DiveGravity = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float DivingRotationSpeed = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float HoveringDistance = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float HoveringSpeed = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Flight")
	float CurrentGroundDistance = 0.0f;
#pragma endregion
    

#pragma region Takeoff & Landing Properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Takeoff & Landing", meta = (ClampMin = "0.0"))
	float TakeOffSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Takeoff & Landing")
	FVector TakeOffDirection = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Takeoff & Landing", meta = (ClampMin = "0.0"))
	float LandingSpeed = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Takeoff & Landing", meta = (ClampMin = "0.0"))
	float LandingRotationSpeed = 600.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Takeoff & Landing")
	FVector TargetLandingLocation;
#pragma endregion

#pragma region Target Properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	AActor* TargetActor = nullptr;
#pragma endregion

#pragma region Event Delegates
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FSFOnStateChanged OnStateChanged;
#pragma endregion

#pragma region Cached Components
private:
	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> CachedCharacterMovementComponent;
#pragma endregion

#pragma region Internal Variables
	float TimeSinceStateChange = 0.0f;
#pragma endregion
};

