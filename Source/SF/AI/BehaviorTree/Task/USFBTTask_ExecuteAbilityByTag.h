// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "GameplayTagContainer.h"
#include "USFBTTask_ExecuteAbilityByTag.generated.h"

class UAbilitySystemComponent;

UCLASS()
class SF_API UUSFBTTask_ExecuteAbilityByTag : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UUSFBTTask_ExecuteAbilityByTag(const FObjectInitializer& ObjectInitializer);

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	// [추가] 안전장치: 시간 체크를 위한 틱 함수
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	void CleanupDelegate(UBehaviorTreeComponent& OwnerComp);

private:
	UAbilitySystemComponent* GetASC(UBehaviorTreeComponent& OwnerComp) const;
	
	void OnTagChanged(const FGameplayTag Tag, int32 NewCount);

	FDelegateHandle EventHandle;
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	// 런타임에 결정된 '기다릴 태그'
	FGameplayTag TagToWait;
	
	bool bFinished = false;
	
	// [추가] 경과 시간
	float ElapsedTime = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Ability")
	FBlackboardKeySelector AbilityTagKey;

	/** * [추가] 감시할 상태 태그 (기본값: Character.State.Attacking)
	 * 이 태그가 사라지면 태스크가 종료됩니다.
	 */
	UPROPERTY(EditAnywhere, Category = "Ability")
	FGameplayTag WaitForTag;

	/** [추가] 최대 대기 시간 (초). 0보다 크면 작동하며, 이 시간이 지나면 강제로 성공 처리합니다. */
	UPROPERTY(EditAnywhere, Category = "Safety")
	float MaxDuration = 5.0f;
};