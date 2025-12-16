// BTService_UpdateTarget.cpp

#include "BTService_UpdateTarget.h"

#include "AIController.h"
#include "AI/Controller/SFEnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
// [제거] #include "AI/SFCombatSlotManager.h"
#include "GameFramework/Pawn.h" // GetPawn() 사용을 위해 필요

UBTService_UpdateTarget::UBTService_UpdateTarget()
{
	NodeName = "Update Target & Slot (Distance Keep)";
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
	
	// 거리가 가까울수록 점수 높음 (단순 선형 계산)
	return FMath::Clamp(1000.f - (Distance / 10.f), 0.f, 1000.f);
}

void UBTService_UpdateTarget::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);
}

void UBTService_UpdateTarget::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
	
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	// 타겟이 살아있다면 슬롯 해제하지 않음 (잠깐 서비스가 꺼진 걸 수도 있음)
	if (BlackboardComp && BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName) != nullptr)
	{
		return;
	}

}

void UBTService_UpdateTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
	if (!AIController) return;

	APawn* MyPawn = AIController->GetPawn();
	if (!MyPawn) return;

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp) return;

	AActor* CurrentTarget = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));

	// ==============================================================================
	// [수정 1] 타겟 유지 판단 (시야보다 거리 우선)
	// 타겟이 이미 있다면, 시야에서 사라져도(등 뒤 등) 일정 거리 안이면 유지합니다.
	// ==============================================================================
	if (CurrentTarget)
	{
		float Dist = FVector::Dist(MyPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
		
		if (Dist > MaxChaseDistance)
		{


			BlackboardComp->ClearValue(TargetActorKey.SelectedKeyName);
			BlackboardComp->SetValueAsBool(HasTargetKey.SelectedKeyName, false);
			AIController->TargetActor = nullptr;
			
			// 타겟을 지웠으니 이번 틱 종료
			return; 
		}
		
	}

	// ==============================================================================
	// [수정 2] 시야 기반 탐색 (새로운 적 찾기 or 타겟 스위칭)
	// ==============================================================================
	TArray<AActor*> PerceivedActors;
	if (auto* PerceptionComp = AIController->GetPerceptionComponent())
	{
		PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
	}

	AActor* BestTarget = nullptr;
	float BestScore = -1.f;

	// 보이는 적들 중 가장 좋은 타겟 선정
	if (PerceivedActors.Num() > 0)
	{
		for (AActor* Actor : PerceivedActors)
		{
			// 태그 체크 (Player 등)
			if (!Actor->ActorHasTag(FName("Player"))) continue;

			const float ActorScore = CalculateTargetScore(OwnerComp, Actor, AIController);
			if (ActorScore > BestScore)
			{
				BestScore = ActorScore;
				BestTarget = Actor;
			}
		}
	}

	// ==============================================================================
	// [수정 3] 타겟 결정 및 갱신 로직
	// ==============================================================================
	
	// Case A: 더 좋은 타겟을 찾음 (BestTarget 존재)
	if (BestTarget)
	{
		bool bShouldSwitch = false;

		// 기존 타겟과 비교
		if (CurrentTarget && BestTarget != CurrentTarget)
		{
			// 기존 타겟 점수 계산 (만약 시야에 없으면 Contains 체크로 알 수 있음)
			// 여기서는 단순히 현재 거리 기준으로 점수 재산출
			float CurrentScore = CalculateTargetScore(OwnerComp, CurrentTarget, AIController);
			
			// 점수 차이가 임계값을 넘어야 교체 (빈번한 교체 방지)
			if ((BestScore - CurrentScore) >= ScoreDifferenceThreshold)
			{
				bShouldSwitch = true;
			}
		}
		// 기존 타겟이 없었으면 무조건 교체
		else if (!CurrentTarget)
		{
			bShouldSwitch = true;
		}

		// 교체 확정
		if (bShouldSwitch)
		{

			CurrentTarget = BestTarget; // 로컬 변수 갱신
			
			// 블랙보드 및 컨트롤러 갱신
			BlackboardComp->SetValueAsObject(TargetActorKey.SelectedKeyName, CurrentTarget);
			BlackboardComp->SetValueAsBool(HasTargetKey.SelectedKeyName, true);
			AIController->TargetActor = CurrentTarget;
			
			// [중요] 타겟을 새로 찾았을 때만 LastKnownPosition 업데이트 (추격용)
			// 시야에서 사라져도 마지막 위치로 가게 하려면 이 부분이 중요함
			BlackboardComp->SetValueAsVector("LastKnownPosition", CurrentTarget->GetActorLocation());
		}
	}
	// Case B: 시야에 적합한 적이 없음 (BestTarget == null)
	else
	{
		// 기존 타겟이 있었다면? -> [수정 1]에서 거리 체크를 통과했으므로 "유지"합니다.
		if (CurrentTarget)
		{
			// 시야에는 없지만 추격 거리 안이므로 유지됨.
			// 아무 작업 안 함 (Keep)
		}
		else
		{
			// 타겟도 없고, 보이는 적도 없음 -> 클리어 (안전 장치)
			BlackboardComp->ClearValue(TargetActorKey.SelectedKeyName);
			BlackboardComp->SetValueAsBool(HasTargetKey.SelectedKeyName, false);
			AIController->TargetActor = nullptr;
			return;
		}
	}

	// ==============================================================================
	// [제거] 슬롯 관리 (Request / Maintain) 로직 제거
	// ==============================================================================
	if (CurrentTarget)
	{
		// 계속 추적 중이라면 위치 정보 업데이트
		BlackboardComp->SetValueAsVector("LastKnownPosition", CurrentTarget->GetActorLocation());
		
	}
}