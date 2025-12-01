// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "Character/Enemy/SFEnemyData.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Net/UnrealNetwork.h" // [추가] 네트워크 관련 헤더
#include "SFEnemyController.generated.h"


class USFEnemyCombatComponent;
class UBehaviorTreeComponent;
class UBlackboardComponent;
class UBehaviorTree;


UCLASS()
class SF_API ASFEnemyController : public AAIController
{
	GENERATED_BODY()

public:
	
	ASFEnemyController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void SetBehaviourContainer(FSFBehaviourWrapperContainer InBehaviorTreeContainer){ BehaviorTreeContainer = InBehaviorTreeContainer; }
	
	void InitializeController();
protected:

	virtual void PreInitializeComponents() override;
	
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void OnPossess(APawn* InPawn) override;

	virtual void OnUnPossess() override;

	virtual void Tick(float DeltaTime) override;

    // [추가] 네트워크 복제 설정
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
    // [수정] Replicated 속성 추가
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
	
    // [추가] 시야 완전 소실 이벤트 (MaxAge 경과)
    UFUNCTION()
    void OnTargetPerceptionForgotten(AActor* Actor);

#pragma endregion

#pragma region Combat
public:
	// 근접 공격 가능한 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat")
	float MeleeRange = 300.f;

	// 경계 거리 (전투 거리 변수명 유지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat")
	float GuardRange = 1200.f;

	// 현재 타겟 유지 우선도 (높을수록 타겟 변경을 덜 함)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat")
	float EngagementBonus = 500.f;

	// 현재 타겟 Actor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Combat")
	TObjectPtr<AActor> TargetActor;

	// 타겟까지 거리 반환
	UFUNCTION(BlueprintCallable, Category="AI|Combat")
	float GetDistanceToTarget() const;

	// 타겟이 근접 공격 범위 안에 있는지
	UFUNCTION(BlueprintCallable, Category="AI|Combat")
	bool IsInMeleeRange() const;

	// 타겟이 원거리 공격 범위 안에 있는지
	UFUNCTION(BlueprintCallable, Category="AI|Combat")
	bool IsInGuardRange() const;

    // [제거됨] FindBestTarget 제거
	// UFUNCTION(BlueprintCallable, Category="AI|Combat")
	// AActor* FindBestTarget();

	//추격 범위 안에 있는가
	UFUNCTION(BlueprintCallable, Category = "AI|Combat")
	bool IsInTrackingRange() const;

protected:
	TObjectPtr<USFEnemyCombatComponent> CombatComponent;
#pragma endregion

#pragma region Debug
	// 시야/범위 디버그 시각화 (콘솔: AI.ShowDebug 1)
	void DrawDebugPerception();
#pragma endregion
	
};