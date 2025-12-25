#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DetourCrowdAIController.h"
#include "GameplayTagContainer.h"
#include "Character/Enemy/SFEnemyData.h"
#include "Interface/SFAIControllerInterface.h"
#include "SFBaseAIController.generated.h"

class USFEnemyCombatComponent;
class UBehaviorTreeComponent;
class UBlackboardComponent;
class UBehaviorTree;


UENUM(BlueprintType)
enum class EAIRotationMode : uint8
{
	None,              // 회전 안 함
	MovementDirection, // 이동 방향으로 회전
	ControllerYaw,     // Controller가 지정한 방향
	TurnInPlace        // 제자리 회전 애니메이션
};

UCLASS(Abstract)
class SF_API ASFBaseAIController : public ADetourCrowdAIController, public ISFAIControllerInterface
{
	GENERATED_BODY()

public:
	ASFBaseAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ISFAIControllerInterface
	virtual void InitializeAIController() override;
	virtual USFEnemyCombatComponent* GetCombatComponent() const override;
	//end

protected:
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#pragma region BehaviorTree

protected:
	// BehaviorTree 변경 (GameplayTag 기반)
	void ChangeBehaviorTree(FGameplayTag GameplayTag);
	
	// BehaviorTree 중지
	void StopBehaviorTree();

	// BehaviorTree 설정 및 실행
	void SetBehaviorTree(UBehaviorTree* BehaviorTree);

	// StateMachine 바인딩 (Pawn의 StateMachine과 연결)
	void BindingStateMachine(const APawn* InPawn);
	
	// StateMachine 언바인딩
	void UnBindingStateMachine();

protected:
	// BehaviorTree 컨테이너 (GameplayTag로 BT 관리)
	UPROPERTY()
	FSFBehaviourWrapperContainer BehaviorTreeContainer;

	// 캐시된 BehaviorTree 컴포넌트
	UPROPERTY()
	TObjectPtr<UBehaviorTreeComponent> CachedBehaviorTreeComponent;

	// 캐시된 Blackboard 컴포넌트
	UPROPERTY()
	TObjectPtr<UBlackboardComponent> CachedBlackboardComponent;

	// 스폰 위치 (복귀 지점으로 사용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|BehaviorTree")
	FVector SpawnLocation;

	// 전투 상태 플래그 (false: Idle / true: Combat)
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="AI|State")
	bool bIsInCombat;
	
	// BehaviorTree 실행 오버라이드
	virtual bool RunBehaviorTree(UBehaviorTree* BehaviorTree) override;
	
#pragma endregion 

#pragma region StateReaction
protected:
	// 상태 이상(CC) 발생 시 BT 일시정지
	UFUNCTION()
	virtual void ReceiveStateStart(FGameplayTag StateTag);
	
	// 상태 이상 해제 시 BT 재개
	UFUNCTION()
	virtual void ReceiveStateEnd(FGameplayTag StateTag);
#pragma endregion 

#pragma region Combat
public:
	// 현재 타겟 Actor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Combat")
	TObjectPtr<AActor> TargetActor;

	// Combat Component (자식 클래스에서 생성)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI|Combat")
	TObjectPtr<USFEnemyCombatComponent> CombatComponent;
#pragma endregion

#pragma region Team
public:
	virtual void SetGenericTeamId(const FGenericTeamId& InTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;

protected:
	UPROPERTY(Replicated)
	FGenericTeamId TeamId;
#pragma endregion

#pragma region Rotation

protected:
	UPROPERTY()
	EAIRotationMode CurrentRotationMode = EAIRotationMode::MovementDirection;

	// Control Rotation 회전 속도 (FocalPoint를 향한 Controller 회전 속도)
	UPROPERTY(EditAnywhere, Category = "AI|Rotation")
	float ControllerYawRotationSpeed = 90.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Rotation")
	float ControlRotationInterpSpeed = 2.5f;

	// 타깃이 근접했을때 미세 회전 멈춤 거리
	UPROPERTY(EditAnywhere, Category = "AI|Rotation")
	float StopRotateDistance = 0.f;

public:
	void SetRotationMode(EAIRotationMode NewMode);

	// 현재 회전 모드
	UFUNCTION(BlueprintCallable, Category = "AI|Rotation")
	EAIRotationMode GetCurrentRotationMode() const { return CurrentRotationMode; }

protected:
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn) override;
	void UpdateControlRotationTowardsFocus(float DeltaTime);
#pragma endregion
};

