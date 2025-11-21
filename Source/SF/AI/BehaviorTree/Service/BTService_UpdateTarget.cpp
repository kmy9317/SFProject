#include "BTService_UpdateTarget.h"
#include "AIController.h"
#include "AI/Controller/SFEnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "AI/SFCombatSlotManager.h" // [기준] SF 접두사가 붙은 매니저 헤더

UBTService_UpdateTarget::UBTService_UpdateTarget()
{
    NodeName = "Update Target (Score Logic)";
    Interval = 0.2f; // 0.2초마다 갱신
    RandomDeviation = 0.05f;

    // 블랙보드 키 필터
    TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateTarget, TargetActorKey), AActor::StaticClass());
    HasTargetKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateTarget, HasTargetKey));
}

float UBTService_UpdateTarget::CalculateTargetScore(UBehaviorTreeComponent& OwnerComp, AActor* Target, ASFEnemyController* AIController) const
{
    if (!Target || !AIController) return -1.f;

    APawn* ControlledPawn = AIController->GetPawn();
    if (!ControlledPawn) return -1.f;

    // 단순 거리 기반 점수 (가까울수록 높음, Max 1000)
    const float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), Target->GetActorLocation());
    return FMath::Clamp(1000.f - (Distance / 10.f), 0.f, 1000.f);
}

void UBTService_UpdateTarget::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::OnBecomeRelevant(OwnerComp, NodeMemory);
    // 활성화 로그 필요시 추가
}

void UBTService_UpdateTarget::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::OnCeaseRelevant(OwnerComp, NodeMemory);
    // 비활성화 로그 필요시 추가
}

void UBTService_UpdateTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    // [중요] SFEnemyController로 캐스팅
    ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
    if (!AIController) return;

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp) return;

    // 현재 타겟 확인
    AActor* CurrentTarget = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));

    // 감지된 모든 액터 가져오기
    TArray<AActor*> PerceivedActors;
    if (AIController->GetPerceptionComponent())
    {
       AIController->GetPerceptionComponent()->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
    }

    // 1. 감지된 적이 아예 없는 경우 -> 타겟 해제
    if (PerceivedActors.Num() == 0)
    {
       if (CurrentTarget)
       {
          UWorld* World = AIController->GetWorld();
          if (World)
          {
             // [수정] SFCombatSlotManager 사용 (U 접두사 포함)
             USFCombatSlotManager* Manager = World->GetSubsystem<USFCombatSlotManager>();
             if (Manager)
             {
                Manager->ReleaseSlot(AIController, CurrentTarget);
             }
          }

          BlackboardComp->ClearValue(TargetActorKey.SelectedKeyName);
          BlackboardComp->SetValueAsBool(HasTargetKey.SelectedKeyName, false);
          AIController->TargetActor = nullptr; // 컨트롤러 변수 동기화
       }
       return;
    }

    // 2. 점수 계산 (최고 점수 타겟 찾기)
    AActor* BestTarget = nullptr;
    float BestScore = -1.f;

    for (AActor* Actor : PerceivedActors)
    {
       // [필터] Player 태그 확인 (필요시 TargetTag 변수로 교체 가능)
       if (!Actor->ActorHasTag(FName("Player"))) continue;

       const float ActorScore = CalculateTargetScore(OwnerComp, Actor, AIController);
       if (ActorScore > BestScore)
       {
          BestScore = ActorScore;
          BestTarget = Actor;
       }
    }

    // 3. 유효한 후보가 없는 경우 처리
    if (!BestTarget)
    {
       if (CurrentTarget)
       {
          // 기존 타겟 해제
          UWorld* World = AIController->GetWorld();
          if (World)
          {
             // [수정] SFCombatSlotManager 사용
             USFCombatSlotManager* Manager = World->GetSubsystem<USFCombatSlotManager>();
             if (Manager)
             {
                Manager->ReleaseSlot(AIController, CurrentTarget);
             }
          }
          BlackboardComp->ClearValue(TargetActorKey.SelectedKeyName);
          BlackboardComp->SetValueAsBool(HasTargetKey.SelectedKeyName, false);
          AIController->TargetActor = nullptr;
       }
       return;
    }

    // 4. 타겟 변경 판정 및 적용
    bool bShouldSwitch = false;

    if (BestTarget != CurrentTarget)
    {
       // 점수 차이 계산 (Hysteresis)
       float CurrentScore = -1.f;
       if (CurrentTarget && PerceivedActors.Contains(CurrentTarget))
       {
          CurrentScore = CalculateTargetScore(OwnerComp, CurrentTarget, AIController);
       }

       if ((BestScore - CurrentScore) >= ScoreDifferenceThreshold)
       {
          bShouldSwitch = true;
       }
    }
    else if (!CurrentTarget) // 현재 타겟이 없는데 베스트 타겟이 있는 경우 (첫 발견)
    {
       bShouldSwitch = true;
    }

    // 5. 실제 변경 실행 (슬롯 매니저 연동)
    if (bShouldSwitch)
    {
       UWorld* World = AIController->GetWorld();
       if (World)
       {
          // [수정] SFCombatSlotManager 사용
          USFCombatSlotManager* Manager = World->GetSubsystem<USFCombatSlotManager>();
          if (Manager)
          {
             // 기존 타겟 슬롯 반납
             if (CurrentTarget)
             {
                Manager->ReleaseSlot(AIController, CurrentTarget);
             }

             // 새 타겟 슬롯 요청
             if (Manager->RequestSlot(AIController, BestTarget))
             {
                // [성공] 블랙보드 및 컨트롤러 업데이트
                BlackboardComp->SetValueAsObject(TargetActorKey.SelectedKeyName, BestTarget);
                BlackboardComp->SetValueAsBool(HasTargetKey.SelectedKeyName, true);
                
                AIController->TargetActor = BestTarget; // SF 컨트롤러 변수 동기화
                AIController->SetFocus(BestTarget, EAIFocusPriority::Gameplay); // 시선 고정

                // 로그 출력 (선택 사항)
                // UE_LOG(LogTemp, Log, TEXT("Target Switched to %s"), *BestTarget->GetName());
             }
             else
             {
                // [실패] 슬롯 꽉 참 -> 기존 타겟이라도 다시 잡아야 함 (만약 있었다면)
                if (CurrentTarget)
                {
                   Manager->RequestSlot(AIController, CurrentTarget);
                }
             }
          }
       }
    }
}