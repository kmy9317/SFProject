// BTService_RunEQSQuery.h

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "BTService_RunEQSQuery.generated.h"

class UEnvQuery;

/**
 * UBTService_RunEQSQuery
 *
 * - 주기적으로 EQS Query를 실행하여 Blackboard 키를 업데이트
 * - bDetectTargetChange: 타겟이 변경되면 즉시 EQS 재실행
 * - NodeMemory를 사용하여 AI별 독립적인 쿼리 상태 관리
 */
UCLASS()
class SF_API UBTService_RunEQSQuery : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_RunEQSQuery();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// EQS Query 완료 콜백
	void OnQueryFinished(TSharedPtr<FEnvQueryResult> Result, UBehaviorTreeComponent* OwnerComp);

	// ========================================
	// 설정
	// ========================================

	/** 실행할 EQS Query 에셋 */
	UPROPERTY(EditAnywhere, Category = "EQS")
	UEnvQuery* QueryTemplate;

	/** 결과를 저장할 Blackboard 키 (Vector) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName ResultKeyName = FName("StrafeLocation");

	/** EQS Query 실행 모드 */
	UPROPERTY(EditAnywhere, Category = "EQS")
	TEnumAsByte<EEnvQueryRunMode::Type> RunMode = EEnvQueryRunMode::SingleResult;

	/** 타겟 변경 감지 활성화 */
	UPROPERTY(EditAnywhere, Category = "EQS")
	bool bDetectTargetChange = true;

	/** 타겟 Actor 키 (타겟 변경 감지용) */
	UPROPERTY(EditAnywhere, Category = "Blackboard", meta = (EditCondition = "bDetectTargetChange"))
	FName TargetActorKeyName = FName("TargetActor");

	// ========================================
	// BTService 오버라이드
	// ========================================
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;

private:
	/**
	 * Per-Instance NodeMemory
	 * 각 AI별 독립적인 상태(이전 타겟, 쿼리 ID) 저장
	 */
	struct FBTRunEQSQueryMemory
	{
		/** 이전 프레임의 타겟 */
		TWeakObjectPtr<AActor> LastTargetActor;
        
        /** 현재 실행 중인 쿼리 ID (중단용) */
        int32 QueryID;

		FBTRunEQSQueryMemory()
			: LastTargetActor(nullptr), QueryID(INDEX_NONE)
		{
		}
	};
};