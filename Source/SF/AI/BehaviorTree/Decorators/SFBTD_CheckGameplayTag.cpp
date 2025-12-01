// Fill out your copyright notice in the Description page of Project Settings.

#include "SFBTD_CheckGameplayTag.h"
#include "AIController.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

USFBTD_CheckGameplayTag::USFBTD_CheckGameplayTag()
{
	NodeName = "Check Gameplay Tag (ASC) Enemy";
    
	// 태그 상태가 변하면 즉시 트리 흐름을 바꾸도록 설정 (매우 중요!)
	// LowerPriority: 이 노드보다 우선순위가 낮은 노드들을 중단시키고 이쪽으로 진입
	FlowAbortMode = EBTFlowAbortMode::LowerPriority;
	bNotifyTick = true; 
}

bool USFBTD_CheckGameplayTag::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return false;

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn) return false;

	// ASC 가져오기 (전역 함수 사용)
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
	if (!ASC) return false;

	// 태그 확인
	// HasMatchingGameplayTag: 해당 태그를 정확히 가지고 있거나, 그 하위 태그를 가지고 있는지 확인
	const bool bHasTag = ASC->HasMatchingGameplayTag(TagToCheck);
    
	// bInvertCondition이 true면 반대 결과 반환 (NOT 연산)
	return bInvertCondition ? !bHasTag : bHasTag;
}

void USFBTD_CheckGameplayTag::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// NodeMemory에서 지난 프레임의 결과 가져오기
	bool* bLastResult = reinterpret_cast<bool*>(NodeMemory);

	// 현재 상태 다시 계산
	bool bCurrentResult = CalculateRawConditionValue(OwnerComp, NodeMemory);

	// 상태가 바뀌었으면 (예: 사거리 밖으로 나감) 트리 재평가 요청 -> Abort 발동
	if (*bLastResult != bCurrentResult)
	{
		OwnerComp.RequestExecution(this);
	}

	// 현재 결과 저장
	*bLastResult = bCurrentResult;
}