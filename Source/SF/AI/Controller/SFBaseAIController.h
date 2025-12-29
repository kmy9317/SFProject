#pragma once
#include "CoreMinimal.h"
#include "DetourCrowdAIController.h"
#include "GameplayTagContainer.h"
#include "Character/Enemy/SFEnemyData.h"
#include "Interface/SFAIControllerInterface.h"
#include "SFBaseAIController.generated.h"

class USFEnemyCombatComponent;
class UBehaviorTreeComponent;
class UBlackboardComponent;
class UBehaviorTree;
class USFTurnInPlaceComponent;



/// 혹시 Enemy만들때 참고
/// 소형 360 ~ 540
/// 중형 







UENUM(BlueprintType)
enum class EAIRotationMode : uint8
{
	None,                 // CC 상태 등
	MovementDirection,    // 비전투 이동
	ControllerYaw        // 전투 모드 (이동+정지 모두 처리)
};

UCLASS(Abstract)
class SF_API ASFBaseAIController 
	: public ADetourCrowdAIController
	, public ISFAIControllerInterface
{
	GENERATED_BODY()

public:
	ASFBaseAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/* ISFAIControllerInterface */
	virtual void InitializeAIController() override;
	virtual USFEnemyCombatComponent* GetCombatComponent() const override;

protected:
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#pragma region BehaviorTree
protected:
	void ChangeBehaviorTree(FGameplayTag GameplayTag);
	void StopBehaviorTree();
	void SetBehaviorTree(UBehaviorTree* BehaviorTree);
	void BindingStateMachine(const APawn* InPawn);
	void UnBindingStateMachine();

	UPROPERTY()
	FSFBehaviourWrapperContainer BehaviorTreeContainer;

	UPROPERTY()
	TObjectPtr<UBehaviorTreeComponent> CachedBehaviorTreeComponent;

	UPROPERTY()
	TObjectPtr<UBlackboardComponent> CachedBlackboardComponent;

	UPROPERTY(Replicated, VisibleAnywhere, Category="AI|State")
	bool bIsInCombat = false;

	virtual bool RunBehaviorTree(UBehaviorTree* BehaviorTree) override;
#pragma endregion

#pragma region StateReaction
protected:
	UFUNCTION()
	void ReceiveStateStart(FGameplayTag StateTag);

	UFUNCTION()
	void ReceiveStateEnd(FGameplayTag StateTag);
#pragma endregion

#pragma region Combat
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Combat")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI|Combat")
	TObjectPtr<USFEnemyCombatComponent> CombatComponent;
#pragma endregion

#pragma region TurnInPlace
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|TurnInPlace")
	TObjectPtr<USFTurnInPlaceComponent> TurnInPlaceComponent;

public:
	UFUNCTION(BlueprintCallable, Category="AI|TurnInPlace")
	USFTurnInPlaceComponent* GetTurnInPlaceComponent() const { return TurnInPlaceComponent; }
#pragma endregion

#pragma region Team
public:
	virtual void SetGenericTeamId(const FGenericTeamId& InTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	void SetbSuppressControlRotationUpdates(bool bInSuppress) { bSuppressControlRotationUpdates = bInSuppress; }
protected:
	UPROPERTY(Replicated)
	FGenericTeamId TeamId;

	bool bSuppressControlRotationUpdates = false;
#pragma endregion

#pragma region Rotation
protected:
	UPROPERTY()
	EAIRotationMode CurrentRotationMode = EAIRotationMode::ControllerYaw;

	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn) override;

	void RestoreRotationAfterAbility();
	void SetMovementBasedRotation(bool bIsMoving);

public:
	void SetRotationMode(EAIRotationMode NewMode);
	EAIRotationMode GetCurrentRotationMode() const { return CurrentRotationMode; }

	void EnsureRotationModeReset();
	void DisableTurnInPlaceFor(float Duration);
	bool CanTurnInPlace() const;

	UFUNCTION(BlueprintCallable, Category="AI|Rotation")
	bool IsTurningInPlace() const;

	//회전 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Rotation", meta=(ClampMin="0.0", ClampMax="1080.0"))
	float MovingRotationRate = 540.f;  // 이동 중 회전 속도 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Rotation", meta=(ClampMin="0.0", ClampMax="1080.0"))
	float StationaryRotationRate = 360.f;  // 정지 중 회전 속도 

private:
	float DisableTurnInPlaceUntilTime = 0.f;
	FTimerHandle RotationRestoreTimerHandle;
	bool bWasMovingLastFrame = false;

	UFUNCTION()
	void OnAbilityStateChanged(FGameplayTag Tag, int32 NewCount);
#pragma endregion
};
