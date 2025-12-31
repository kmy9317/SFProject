// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "SFBTTask_ApproachAndAttack.generated.h"

class UAbilitySystemComponent;

/**
 * [순서]
 * 1. 회전 (Rotate): 타겟을 바라볼 때까지 부드럽게 회전
 * 2. 이동 (Approach): 사거리 밖이라면 타겟에게 접근
 * 3. 공격 (Attack): 사거리 내라면 멈추고 어빌리티 실행
 */
UCLASS()
class SF_API USFBTTask_ApproachAndAttack : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	USFBTTask_ApproachAndAttack(const FObjectInitializer& ObjectInitializer);

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

private:
	// 내부 헬퍼 함수
	UAbilitySystemComponent* GetASC(UBehaviorTreeComponent& OwnerComp) const;
	
	// 회전 후 이동 또는 공격 결정
	void StartApproachOrAttack(UBehaviorTreeComponent& OwnerComp);
	
	// 실제 공격 실행
	void PerformAttack(UBehaviorTreeComponent& OwnerComp);
	
	void OnTagChanged(const FGameplayTag Tag, int32 NewCount);
	void CleanupDelegate(UBehaviorTreeComponent& OwnerComp);

	// 내부 상태 변수
	bool bIsRotating = false;  // 회전 중인가? (1단계)
	bool bIsMoving = false;    // 이동 중인가? (2단계)
	bool bIsAttacking = false; // 공격 후 대기 중인가? (3단계)
	bool bFinished = false;    // 태스크 종료 플래그
	
	float ElapsedTime = 0.0f;
	FDelegateHandle EventHandle;
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
	
	// 런타임에 결정되는 '기다릴 태그'
	FGameplayTag TagToWait; 

public:
	// [설정] 실행할 어빌리티 클래스 (내려찍기, 찌르기 등 BT에서 선택)
	UPROPERTY(EditAnywhere, Category = "Attack")
	TSubclassOf<UGameplayAbility> AbilityClassToRun;

	// [설정] 추적할 타겟 (블랙보드 키)
	UPROPERTY(EditAnywhere, Category = "Attack")
	FBlackboardKeySelector TargetActorKey;

	// [설정] 공격 사거리 (이 거리 안으로 들어오면 이동 멈춤)
	UPROPERTY(EditAnywhere, Category = "Attack")
	float AttackRadius = 150.0f;

	// [설정] 회전 속도 (Interpolation Speed)
	UPROPERTY(EditAnywhere, Category = "Attack")
	float RotationSpeed = 5.0f;

	// [설정] 타겟을 바라보는 것으로 간주할 각도 오차 (이 각도 이내로 들어오면 이동/공격 시작)
	UPROPERTY(EditAnywhere, Category = "Attack")
	float FacingThreshold = 10.0f;

	// [옵션] 공격 상태 태그 (이 태그가 사라지면 공격이 끝난 것으로 간주)
	UPROPERTY(EditAnywhere, Category = "Attack")
	FGameplayTag WaitForTag;

	// [안전장치] 최대 제한 시간
	UPROPERTY(EditAnywhere, Category = "Safety")
	float MaxDuration = 5.0f;
};