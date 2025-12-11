// SFBTS_UpdateFocus.cpp
#include "SFBTS_UpdateFocus.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

USFBTS_UpdateFocus::USFBTS_UpdateFocus()
{
	NodeName = "Update Focus (Look At Target)";
	bNotifyTick = true;
	bNotifyCeaseRelevant = true;
	
	// 반응성을 위해 틱 간격을 짧게 잡거나 0(매 프레임)으로 설정
	// SetFocus 자체는 무겁지 않으므로 0.1~0.2 정도면 충분합니다.
	Interval = 0.1f; 
	RandomDeviation = 0.0f;

	// 블랙보드 키 필터 (Object/Actor만 허용)
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(USFBTS_UpdateFocus, BlackboardKey), AActor::StaticClass());
}

void USFBTS_UpdateFocus::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	if (!AIC || !Blackboard) return;

	// 블랙보드에서 현재 타겟 가져오기
	AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKey.SelectedKeyName));

	// 현재 포커스 중인 액터 가져오기
	AActor* CurrentFocus = AIC->GetFocusActor();

	// 타겟이 유효하고, 현재 포커스와 다르다면 갱신
	if (TargetActor)
	{
		if (CurrentFocus != TargetActor)
		{
			// [핵심] 타겟을 바라보도록 설정 (엔진이 알아서 회전시킴)
			AIC->SetFocus(TargetActor, EAIFocusPriority::Gameplay);
		}
	}
	else
	{
		// 타겟이 없다면 포커스 해제 (선택 사항: 정면을 보게 할지 등)
		if (CurrentFocus)
		{
			AIC->ClearFocus(EAIFocusPriority::Gameplay);
		}
	}
}

void USFBTS_UpdateFocus::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);

	// 서비스가 끝날 때(예: 전투 종료, 트리 변경) 포커스를 풀 것인가?
	// 상황에 따라 다르지만, 보통 전투 상태를 벗어나면 풀어주는 것이 좋습니다.
	if (AAIController* AIC = OwnerComp.GetAIOwner())
	{
		AIC->ClearFocus(EAIFocusPriority::Gameplay);
	}
}