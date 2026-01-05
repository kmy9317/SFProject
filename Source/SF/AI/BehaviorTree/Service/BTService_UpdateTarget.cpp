// SF/AI/BehaviorTree/Service/BTService_UpdateTarget.cpp

#include "BTService_UpdateTarget.h"

// Engine & AI
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "GameFramework/Pawn.h"

// [중요] EnemyController의 SetTargetForce를 쓰기 위해 헤더 포함
#include "SF/AI/Controller/SFEnemyController.h"

// GAS & Custom Headers
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagAssetInterface.h"
#include "SF/Character/SFCharacterGameplayTags.h"

UBTService_UpdateTarget::UBTService_UpdateTarget()
{
	NodeName = "Update Target (Integrated)";
	
	// 반응 속도: 0.1s
	Interval = 0.1f; 
	RandomDeviation = 0.0f;

	// 거리 우선 즉시 교체
	ScoreDifferenceThreshold = 0.0f; 

	// 블랙보드 필터
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateTarget, TargetActorKey), AActor::StaticClass());
	HasTargetKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateTarget, HasTargetKey));
	DistanceToTargetKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateTarget, DistanceToTargetKey));
	LastKnownPositionKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateTarget, LastKnownPositionKey));
}

bool UBTService_UpdateTarget::IsTargetValid(AActor* TargetActor) const
{
	if (!IsValid(TargetActor)) return false;

	const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(TargetActor);
	if (TagInterface)
	{
		if (TagInterface->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead)) return false;
		if (TagInterface->HasMatchingGameplayTag(SFGameplayTags::Character_State_Invulnerable)) return false;
	}
	
	return true;
}

float UBTService_UpdateTarget::CalculateTargetScore(APawn* MyPawn, AActor* Target) const
{
	if (!MyPawn || !Target) return -1.f;
	float Distance = FVector::Dist(MyPawn->GetActorLocation(), Target->GetActorLocation());
	return FMath::Clamp(2000.f - (Distance / 5.f), 0.f, 2000.f);
}

void UBTService_UpdateTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	
	if (!AIController || !Blackboard) return;

	APawn* MyPawn = AIController->GetPawn();
	if (!MyPawn) return;

	// [Check 0] 공격 중 처리 (거리 갱신만 수행)
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MyPawn))
	{
		if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_UsingAbility))
		{
			AActor* LockedTarget = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
			if (LockedTarget)
			{
				float Dist = FVector::Dist(MyPawn->GetActorLocation(), LockedTarget->GetActorLocation());
				if (DistanceToTargetKey.SelectedKeyName != NAME_None)
				{
					Blackboard->SetValueAsFloat(DistanceToTargetKey.SelectedKeyName, Dist);
				}
			}
			return; // 타겟 변경 로직 스킵
		}
	}

	// [Check 1] 현재 타겟 상태 검증
	AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
	
	if (CurrentTarget)
	{
		float Dist = FVector::Dist(MyPawn->GetActorLocation(), CurrentTarget->GetActorLocation());

		if (Dist > MaxChaseDistance || !IsTargetValid(CurrentTarget))
		{
			CurrentTarget = nullptr; 
		}
	}

	// [Check 2] Perception으로 감지된 적 탐색
	TArray<AActor*> PerceivedActors;
	if (UAIPerceptionComponent* PerceptionComp = AIController->GetPerceptionComponent())
	{
		PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
	}

	AActor* BestTarget = nullptr;
	float BestScore = -1.f;

	for (AActor* Actor : PerceivedActors)
	{
		if (!Actor) continue;
		if (!Actor->ActorHasTag(TargetTag)) continue;
		if (!IsTargetValid(Actor)) continue;

		float Score = CalculateTargetScore(MyPawn, Actor);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Actor;
		}
	}

	// [Check 3] 타겟 교체 판단
	bool bShouldUpdate = false;

	if (BestTarget)
	{
		if (!CurrentTarget)
		{
			CurrentTarget = BestTarget;
			bShouldUpdate = true;
		}
		else if (CurrentTarget != BestTarget)
		{
			float CurrentScore = CalculateTargetScore(MyPawn, CurrentTarget);
			if ((BestScore - CurrentScore) >= ScoreDifferenceThreshold)
			{
				CurrentTarget = BestTarget;
				bShouldUpdate = true;
			}
		}
	}
	else if (!CurrentTarget)
	{
		bShouldUpdate = true; 
	}

	// [Check 4] 블랙보드 및 컨트롤러 업데이트
	if (CurrentTarget)
	{
		// 타겟 변경 시
		if (Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName) != CurrentTarget)
		{
			Blackboard->SetValueAsObject(TargetActorKey.SelectedKeyName, CurrentTarget);
			Blackboard->SetValueAsBool(HasTargetKey.SelectedKeyName, true);
			AIController->SetFocus(CurrentTarget);
			
			// ★ [핵심] 컨트롤러의 SetTargetForce 직접 호출
			if (ASFEnemyController* EnemyController = Cast<ASFEnemyController>(AIController))
			{
				EnemyController->SetTargetForce(CurrentTarget);
			}
		}
		
		// 위치와 거리 정보는 매 프레임(Tick) 갱신
		float DistToTarget = FVector::Dist(MyPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
		Blackboard->SetValueAsVector(LastKnownPositionKey.SelectedKeyName, CurrentTarget->GetActorLocation());
		
		if (DistanceToTargetKey.SelectedKeyName != NAME_None)
		{
			Blackboard->SetValueAsFloat(DistanceToTargetKey.SelectedKeyName, DistToTarget);
		}
	}
	else if (bShouldUpdate) // 타겟 잃음
	{
		Blackboard->ClearValue(TargetActorKey.SelectedKeyName);
		Blackboard->SetValueAsBool(HasTargetKey.SelectedKeyName, false);
		
		if (DistanceToTargetKey.SelectedKeyName != NAME_None)
		{
			Blackboard->ClearValue(DistanceToTargetKey.SelectedKeyName);
		}
		
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		
		// 컨트롤러 타겟 해제 시도 (필요하다면 CombatComponent 직접 접근 등 추가 가능하나 SetTargetForce는 null 체크로 리턴됨)
		// 일반적인 경우 여기서 ClearFocus만으로 충분할 수 있습니다.
	}
}