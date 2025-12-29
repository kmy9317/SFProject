// SFBTS_UpdateFocus.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "SFBTS_UpdateFocus.generated.h"

/**
 * 블랙보드의 TargetActor를 지속적으로 바라보게(SetFocus) 하는 서비스
 * 상위 노드에 부착하여 전투 중 항상 타겟을 주시하도록 함
 */
UCLASS()
class SF_API USFBTS_UpdateFocus : public UBTService_BlackboardBase
{
	GENERATED_BODY()

public:
	USFBTS_UpdateFocus();
	void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	void UpdateFocusTarget(UBehaviorTreeComponent& OwnerComp);
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};