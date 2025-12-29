// SFBTS_UpdateFocus.cpp
#include "SFBTS_UpdateFocus.h"
#include "AI/Controller/SFBaseAIController.h" // BaseAIController 헤더 추가
#include "BehaviorTree/BlackboardComponent.h"

USFBTS_UpdateFocus::USFBTS_UpdateFocus()
{
    NodeName = "Update Focus (Look At Target)";
    bNotifyTick = true;
    bNotifyBecomeRelevant = true; // 서비스 시작 시점 감지 추가
    bNotifyCeaseRelevant = true;
    
    Interval = 0.1f; 
    RandomDeviation = 0.02f; // 약간의 무작위성을 주어 성능 분산

    BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(USFBTS_UpdateFocus, BlackboardKey), AActor::StaticClass());
}

void USFBTS_UpdateFocus::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::OnBecomeRelevant(OwnerComp, NodeMemory);
    
    // 서비스가 시작될 때 즉시 한번 체크
    UpdateFocusTarget(OwnerComp);
}

void USFBTS_UpdateFocus::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
    UpdateFocusTarget(OwnerComp);
}

void USFBTS_UpdateFocus::UpdateFocusTarget(UBehaviorTreeComponent& OwnerComp)
{
    ASFBaseAIController* AIC = Cast<ASFBaseAIController>(OwnerComp.GetAIOwner());
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

    if (!AIC || !Blackboard) return;

    // ✅ 1. Turn In Place 중에는 Focus 업데이트 중단
    if (AIC->IsTurningInPlace())
    {
        return;
    }

    // ✅ 2. None 모드(다른 Ability 실행 중)에서도 중단
    if (AIC->GetCurrentRotationMode() == EAIRotationMode::None)
    {
        return;
    }

    AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKey.SelectedKeyName));
    AActor* CurrentFocus = AIC->GetFocusActor();

    if (TargetActor)
    {
        if (CurrentFocus != TargetActor)
        {
            // ✅ 3. SetFocus만 호출 (RotationMode 전환은 하지 않음)
            // Controller의 SetFocus가 자동으로 ControllerYaw로 전환하도록 수정
            AIC->SetFocus(TargetActor, EAIFocusPriority::Gameplay);
        }
    }
    else
    {
        if (CurrentFocus)
        {
            AIC->ClearFocus(EAIFocusPriority::Gameplay);
            AIC->SetRotationMode(EAIRotationMode::MovementDirection);
        }
    }
}

void USFBTS_UpdateFocus::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 서비스 범위를 벗어날 때(노드 종료 시) 포커스 해제
    if (AAIController* AIC = OwnerComp.GetAIOwner())
    {
       AIC->ClearFocus(EAIFocusPriority::Gameplay);
    }
    
    Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}