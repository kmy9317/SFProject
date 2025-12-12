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

/**
 * Base AI Controller with common BehaviorTree, StateMachine, and StateReaction functionality
 * Used as a base class for both regular enemies and special enemies like Dragon
 */
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
public:
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="AI|State")
	bool bHasGuardSlot = false;

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
	UPROPERTY()
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
};

