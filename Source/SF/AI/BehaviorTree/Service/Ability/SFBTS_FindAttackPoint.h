// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "SFBTS_FindAttackPoint.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFBTS_FindAttackPoint : public UBTService
{
	GENERATED_BODY()

public:
	USFBTS_FindAttackPoint();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// EQS Query 완료 콜백
	void OnQueryFinished(TSharedPtr<FEnvQueryResult> Result, UBehaviorTreeComponent* OwnerComp);
	

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
	
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;

private:
	/**
	 * Per-Instance NodeMemory
	 * 각 AI별 독립적인 상태(이전 타겟, 쿼리 ID) 저장
	 */
	struct FBTRunEQSQueryMemory
	{
		TWeakObjectPtr<AActor> LastTargetActor;
		
		int32 QueryID;
		
		FGameplayTag LastAbilityTag;

		FBTRunEQSQueryMemory()
			: LastTargetActor(nullptr), QueryID(INDEX_NONE), LastAbilityTag()
		{
		}
	};
};
