// SF/AI/BehaviorTree/Service/BTService_UpdateTarget.cpp

#include "BTService_UpdateTarget.h"

#include "AIController.h"
#include "AI/Controller/SFEnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "GameFramework/Pawn.h"

UBTService_UpdateTarget::UBTService_UpdateTarget()
{
	NodeName = "Update Target";
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
	// 가까울수록 높은 점수
	return FMath::Clamp(1000.f - (Distance / 10.f), 0.f, 1000.f);
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

	// 1. 현재 타겟과의 거리 체크
	// (MaxChaseDistance를 999999로 늘렸으므로, 이제 여기서 타겟이 갑자기 사라지지 않습니다)
	if (CurrentTarget)
	{
		float Dist = FVector::Dist(MyPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
		
		if (Dist > MaxChaseDistance)
		{
			BlackboardComp->ClearValue(TargetActorKey.SelectedKeyName);
			BlackboardComp->SetValueAsBool(HasTargetKey.SelectedKeyName, false);
			AIController->TargetActor = nullptr;

			//  CombatComponent도 클리어
			if (AIController->CombatComponent && CurrentTarget)
			{
				AIController->CombatComponent->HandleTargetPerceptionUpdated(CurrentTarget, false);
			}

			// 타겟을 지웠으니 이번 틱 종료
			return;
		}
	}

	// 2. 시야(Perception)에 보이는 적들 탐색
	TArray<AActor*> PerceivedActors;
	if (auto* PerceptionComp = AIController->GetPerceptionComponent())
	{
		PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
	}

	AActor* BestTarget = nullptr;
	float BestScore = -1.f;

	if (PerceivedActors.Num() > 0)
	{
		for (AActor* Actor : PerceivedActors)
		{
			// 플레이어 태그 확인
			if (!Actor->ActorHasTag(FName("Player"))) continue;

			const float ActorScore = CalculateTargetScore(OwnerComp, Actor, AIController);
			if (ActorScore > BestScore)
			{
				BestScore = ActorScore;
				BestTarget = Actor;
			}
		}
	}

	// 3. 타겟 갱신 로직 (더 좋은 타겟이 있거나, 타겟이 없을 때)
	if (BestTarget)
	{
		bool bShouldSwitch = false;

		if (CurrentTarget && BestTarget != CurrentTarget)
		{
			float CurrentScore = CalculateTargetScore(OwnerComp, CurrentTarget, AIController);
			// 점수 차이가 날 때만 교체
			if ((BestScore - CurrentScore) >= ScoreDifferenceThreshold)
			{
				bShouldSwitch = true;
			}
		}
		else if (!CurrentTarget)
		{
			bShouldSwitch = true;
		}

		if (bShouldSwitch)
		{

			CurrentTarget = BestTarget; // 로컬 변수 갱신

			// 블랙보드 및 컨트롤러 갱신
			BlackboardComp->SetValueAsObject(TargetActorKey.SelectedKeyName, CurrentTarget);
			BlackboardComp->SetValueAsBool(HasTargetKey.SelectedKeyName, true);
			AIController->TargetActor = CurrentTarget;

			//  CombatComponent도 업데이트 (UpdateDesiredControlYaw가 올바른 타겟 사용)
			if (AIController->CombatComponent)
			{
				AIController->CombatComponent->HandleTargetPerceptionUpdated(CurrentTarget, true);
			}

			// [중요] 타겟을 새로 찾았을 때만 LastKnownPosition 업데이트 (추격용)
			// 시야에서 사라져도 마지막 위치로 가게 하려면 이 부분이 중요함
			BlackboardComp->SetValueAsVector("LastKnownPosition", CurrentTarget->GetActorLocation());
		}
	}
	else if (!CurrentTarget)
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

			//  CombatComponent도 클리어
			if (AIController->CombatComponent && CurrentTarget)
			{
				AIController->CombatComponent->HandleTargetPerceptionUpdated(CurrentTarget, false);
			}

			return;
		}
	}

	// 타겟 추적 중이면 마지막 위치 업데이트
	if (CurrentTarget)
	{
		BlackboardComp->SetValueAsVector("LastKnownPosition", CurrentTarget->GetActorLocation());
	}
}