// SF/AI/BehaviorTree/Task/SFBTTask_ApproachAndAttack.h

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "SFBTTask_ApproachAndAttack.generated.h"

class UAbilitySystemComponent;

/**
 * [복합 태스크] 접근 후 공격
 * 순서:
 * 1. 회전 (Rotate): 타겟을 바라볼 때까지 부드럽게 회전
 * 2. 이동 (Approach): 사거리 밖이라면 타겟에게 접근
 * 3. 공격 (Attack): 사거리 내라면 멈추고 어빌리티 실행
 * * [수정사항] 타겟이 죽었는지(Dead 태그) 수시로 확인하여, 죽었다면 즉시 중단.
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
	void StartApproachOrAttack(UBehaviorTreeComponent& OwnerComp);
	
	// 실제 공격 실행
	void PerformAttack(UBehaviorTreeComponent& OwnerComp);
	
	void OnTagChanged(const FGameplayTag Tag, int32 NewCount);
	void CleanupDelegate(UBehaviorTreeComponent& OwnerComp);

	// [추가] 타겟 생존 여부 확인 함수
	bool IsTargetAlive(AActor* Target) const;

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

	// [설정] 공격 사거리 (이 거리 안으로 들어오면 공격)
	UPROPERTY(EditAnywhere, Category = "Attack")
	float AttackRadius = 150.0f;

	// [설정] 회전 속도
	UPROPERTY(EditAnywhere, Category = "Rotate")
	float RotationSpeed = 5.0f;

	// [설정] 정면 판정 각도 (이 각도 안으로 들어오면 회전 끝)
	UPROPERTY(EditAnywhere, Category = "Rotate")
	float FacingThreshold = 10.0f;

	// [설정] 공격 애니메이션이 끝났다고 판단할 태그 (기본: Character.State.Attacking)
	// 이 태그가 사라지면 태스크 종료
	UPROPERTY(EditAnywhere, Category = "Attack")
	FGameplayTag WaitForTag;

	// [설정] 최대 대기 시간 (버그 방지용 타임아웃)
	UPROPERTY(EditAnywhere, Category = "Attack")
	float MaxDuration = 5.0f;
};