// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DetourCrowdAIController.h" // [수정] 헤더 변경 (AIController.h -> DetourCrowdAIController.h)
#include "GameplayTagContainer.h"
#include "Character/Enemy/SFEnemyData.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Net/UnrealNetwork.h"
#include "SFEnemyController.generated.h"


class USFEnemyCombatComponent;
class UBehaviorTreeComponent;
class UBlackboardComponent;
class UBehaviorTree;


// [수정] 부모 클래스 변경: AAIController -> ADetourCrowdAIController
UCLASS()
class SF_API ASFEnemyController : public ADetourCrowdAIController
{
	GENERATED_BODY()

public:
	
	ASFEnemyController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void SetBehaviourContainer(FSFBehaviourWrapperContainer InBehaviorTreeContainer){ BehaviorTreeContainer = InBehaviorTreeContainer; }

	USFEnemyCombatComponent* GetCombatComponent() const { return CombatComponent; }

	void InitializeController();
protected:

	virtual void PreInitializeComponents() override;
	
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void OnPossess(APawn* InPawn) override;

	virtual void OnUnPossess() override;

	virtual void Tick(float DeltaTime) override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#pragma region BehaviorTree
public:
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="AI|State")
	bool bHasGuardSlot = false;
protected:
	
	void ChangeBehaviorTree(FGameplayTag GameplayTag);
	
	void StopBehaviorTree();

	void SetBehaviorTree(UBehaviorTree* BehaviorTree);

	void BindingStateMachine(const APawn* InPawn);
	
	void UnBindingStateMachine();

protected:
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

#pragma region tagbinding
protected:
	UFUNCTION()
	void ReceiveStateStart(FGameplayTag StateTag);
	UFUNCTION()
	void ReceiveStateEnd(FGameplayTag StateTag);
#pragma endregion 
#pragma region Perception
	
	// 시야 감지 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	// 시야 감지 구조체
	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	// 감지 가능 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception")
	float SightRadius = 2000.f;

	// 감지 유지 종료 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception")
	float LoseSightRadius = 3500.f;

	// Idle 상태 시 시야각 (45도 = 정면 약 90도 부채꼴)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception")
	float PeripheralVisionAngleDegrees = 45.f;

	// 액터태그 기반 타겟 액터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception")
	FName TargetTag = FName("Player");

	// 시야 감지 이벤트 콜백 (감지/상실)
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
    // 시야 완전 소실 이벤트 (MaxAge 경과)
    UFUNCTION()
    void OnTargetPerceptionForgotten(AActor* Actor);

#pragma endregion

#pragma region Combat
public:

	// 현재 타겟 Actor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Combat")
	TObjectPtr<AActor> TargetActor;

	TObjectPtr<USFEnemyCombatComponent> CombatComponent;
#pragma endregion

#pragma region Debug
	// 시야/범위 디버그 시각화 (콘솔: AI.ShowDebug 1)
	void DrawDebugPerception();
#pragma endregion
	
};