// BTService_UpdateTarget.cpp

#include "BTService_UpdateTarget.h"

#include "AIController.h"
#include "AI/Controller/SFEnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "AI/SFCombatSlotManager.h"

UBTService_UpdateTarget::UBTService_UpdateTarget()
{
	NodeName = "Update Target & Slot (No Focus)";
	// 타겟 탐색은 조금 천천히 해도 됨 (Focus 서비스와 분리되었으므로 최적화 가능)
	Interval = 0.2f; 
	RandomDeviation = 0.05f;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateTarget, TargetActorKey), AActor::StaticClass());
	HasTargetKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateTarget, HasTargetKey));
}

float UBTService_UpdateTarget::CalculateTargetScore(UBehaviorTreeComponent& OwnerComp, AActor* Target, ASFEnemyController* AIController) const
{
	if (!Target || !AIController) return -1.f;

	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn) return -1.f;

	const float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), Target->GetActorLocation());
	
	// 거리가 가까울수록 점수 높음
	return FMath::Clamp(1000.f - (Distance / 10.f), 0.f, 1000.f);
}

void UBTService_UpdateTarget::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);
}

void UBTService_UpdateTarget::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
	
	// 서비스 종료 시 슬롯 해제 로직
	// 단, 타겟이 블랙보드에 남아있다면(잠깐 다른 노드 다녀오는 거라면) 유지함
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp && BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName) != nullptr)
	{
		return;
	}
	
	ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
	if (AIController)
	{
		if (UWorld* World = AIController->GetWorld())
		{
			if (USFCombatSlotManager* Manager = World->GetSubsystem<USFCombatSlotManager>())
			{
				Manager->ReleaseSlot(AIController);
			}
		}
	}
}

void UBTService_UpdateTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
	if (!AIController) return;

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp) return;

	AActor* CurrentTarget = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));

	// 감지된 적 목록 가져오기
	TArray<AActor*> PerceivedActors;
	if (auto* PerceptionComp = AIController->GetPerceptionComponent())
	{
		PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
	}

	// 1. 적이 아예 안 보이면 -> 타겟 해제 & 슬롯 반납
	if (PerceivedActors.Num() == 0)
	{
		if (CurrentTarget)
		{
			if (auto* Manager = AIController->GetWorld()->GetSubsystem<USFCombatSlotManager>())
				Manager->ReleaseSlot(AIController, CurrentTarget);

			BlackboardComp->ClearValue(TargetActorKey.SelectedKeyName);
			BlackboardComp->SetValueAsBool(HasTargetKey.SelectedKeyName, false);
			AIController->TargetActor = nullptr; 
		}
		return;
	}

	// 2. 타겟 점수 계산
	AActor* BestTarget = nullptr;
	float BestScore = -1.f;

	for (AActor* Actor : PerceivedActors)
	{
		if (!Actor->ActorHasTag(FName("Player"))) continue;

		const float ActorScore = CalculateTargetScore(OwnerComp, Actor, AIController);
		if (ActorScore > BestScore)
		{
			BestScore = ActorScore;
			BestTarget = Actor;
		}
	}

	// 3. 유효 타겟 없음 -> 해제
	if (!BestTarget)
	{
		if (CurrentTarget)
		{
			if (auto* Manager = AIController->GetWorld()->GetSubsystem<USFCombatSlotManager>())
				Manager->ReleaseSlot(AIController, CurrentTarget);
			
			BlackboardComp->ClearValue(TargetActorKey.SelectedKeyName);
			BlackboardComp->SetValueAsBool(HasTargetKey.SelectedKeyName, false);
			AIController->TargetActor = nullptr;
		}
		return;
	}

	// 4. 타겟 변경 여부 결정
	bool bShouldSwitch = false;
	if (BestTarget != CurrentTarget)
	{
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
	else if (!CurrentTarget)
	{
		bShouldSwitch = true;
	}

	// 5. 타겟 정보 업데이트
	if (bShouldSwitch)
	{
		// 기존 타겟 슬롯 반납
		if (CurrentTarget) 
		{
			if (auto* Manager = AIController->GetWorld()->GetSubsystem<USFCombatSlotManager>())
				Manager->ReleaseSlot(AIController, CurrentTarget);
		}

		BlackboardComp->SetValueAsObject(TargetActorKey.SelectedKeyName, BestTarget);
		BlackboardComp->SetValueAsBool(HasTargetKey.SelectedKeyName, true);
		
		AIController->TargetActor = BestTarget;
		
		// [변경됨] 직접 SetFocus 하던 코드를 제거함. 
		// 이제 SFBTS_UpdateFocus 서비스가 이 역할을 전담합니다.
		// AIController->SetFocus(BestTarget, EAIFocusPriority::Gameplay); <--- 삭제됨
		
		CurrentTarget = BestTarget;
	}

	// 6. 슬롯 요청 및 유지 (Request/Maintain)
	if (CurrentTarget)
	{
		if (auto* Manager = AIController->GetWorld()->GetSubsystem<USFCombatSlotManager>())
		{
			// 거리 체크: 너무 가까우면 강제 공격 (bForce = true)
			bool bForce = false;
			if (APawn* MyPawn = AIController->GetPawn())
			{
				const float Dist = FVector::Dist(MyPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
				if (Dist <= ForceAttackDistance)
				{
					bForce = true;
				}
			}

			// 슬롯 요청 (없으면 요청, 있으면 유지)
			Manager->RequestSlot(AIController, CurrentTarget, bForce);
		}
	}
}