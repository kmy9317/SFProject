#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "SFBTTask_SetupBossPhase.generated.h"

UCLASS()
class SF_API USFBTTask_SetupBossPhase : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USFBTTask_SetupBossPhase();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted, UBehaviorTreeComponent* OwnerComp);
	void FinishAndSetFlag(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type Result);

protected:
	// --- 블랙보드 및 기존 설정 ---
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector PhaseInitializedKey;

	UPROPERTY(EditAnywhere, Category = "Phase Settings|Animation")
	TSubclassOf<UAnimInstance> PhaseAnimLayer;

	UPROPERTY(EditAnywhere, Category = "Phase Settings|Animation")
	TObjectPtr<UAnimMontage> PhaseStartMontage;

	UPROPERTY(EditAnywhere, Category = "Phase Settings|Animation")
	bool bWaitForMontageEnd = true;

	// --- [권장 수치 적용된 이동 설정] ---
	
	// 목표 이동 속도 (뛰는 속도)
	UPROPERTY(EditAnywhere, Category = "Phase Settings|Movement")
	float NewMaxWalkSpeed = 600.0f; 

	// 가속도 (기본값 2400.0f - 속도 600 도달까지 약 0.25초)
	// 이 값이 낮으면 발은 뛰는데 몸이 천천히 출발하는 '문워크'가 발생합니다.
	UPROPERTY(EditAnywhere, Category = "Phase Settings|Movement")
	float NewMaxAcceleration = 2400.0f;

	// 제동력 (기본값 2048.0f)
	// 이 값이 낮으면 멈출 때 스케이트 타듯이 미끄러집니다.
	UPROPERTY(EditAnywhere, Category = "Phase Settings|Movement")
	float NewBrakingDeceleration = 2048.0f; 

	// 회전 속도 (기본값 720.0f - 초당 2바퀴)
	// 보스가 플레이어를 놓치지 않고 빠르게 고개를 돌립니다.
	UPROPERTY(EditAnywhere, Category = "Phase Settings|Movement")
	float NewRotationRateYaw = 720.0f;

	// 지면 마찰력 (기본값 8.0f)
	// 방향 전환 시 관성을 제어합니다.
	UPROPERTY(EditAnywhere, Category = "Phase Settings|Movement")
	float NewGroundFriction = 8.0f;

private:
	bool bIsPlayingMontage = false;
};