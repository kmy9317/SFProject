// SFBTTask_FindAttackPoint.cpp

#include "SFBTTask_FindAttackPoint.h"
#include "AI/Controller/SFEnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"
#include "GameplayTagContainer.h"
#include "NavigationSystem.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/SFCharacterBase.h"

USFBTTask_FindAttackPoint::USFBTTask_FindAttackPoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = "Find Attack Point(Task)";
	bNotifyTaskFinished = true;
	bNotifyTick = false;

    // [핵심 수정] 이 옵션이 없으면 여러 AI가 QueryID 변수를 공유해서 무한 대기에 빠집니다.
	bCreateNodeInstance = true; 
}

EBTNodeResult::Type USFBTTask_FindAttackPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (!QueryTemplate) return EBTNodeResult::Failed;

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp) return EBTNodeResult::Failed;

	ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
	if (!AIController) return EBTNodeResult::Failed;

	FName SelectedAbilityTagName = BlackboardComp->GetValueAsName(AbilityTagKeyName);
	if (SelectedAbilityTagName == NAME_None) return EBTNodeResult::Failed;

	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKeyName));
	if (!TargetActor) return EBTNodeResult::Failed;
	
	FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(SelectedAbilityTagName);
	if (!AbilityTag.IsValid()) return EBTNodeResult::Failed;
	
	float MinDist = 0.0f;
	float MaxDist = 1000.0f;
	USFGA_Enemy_BaseAttack* BaseAttack = nullptr;

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(AIController->GetPawn());
	if (Character)
	{
		USFAbilitySystemComponent* ASC = Character->GetSFAbilitySystemComponent();
		if (ASC)
		{
			for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
			{
				if (Spec.Ability)
				{
					bool bTagMatches = Spec.Ability->AbilityTags.HasTag(AbilityTag) || 
									   Spec.Ability->GetAssetTags().HasTag(AbilityTag);

					if (bTagMatches)
					{
						UGameplayAbility* AbilityInstance = Spec.GetPrimaryInstance();
						if (!AbilityInstance) AbilityInstance = Spec.Ability;

						BaseAttack = Cast<USFGA_Enemy_BaseAttack>(AbilityInstance);
						if (BaseAttack)
						{
							MinDist = BaseAttack->GetMinAttackRange();
							MaxDist = BaseAttack->GetAttackRange();
							break;
						}
					}
				}
			}
		}
	}
	
    // 이미 범위 내라면 즉시 성공
	if (bSkipIfInRange && BaseAttack && TargetActor)
	{
		if (BaseAttack->IsWithinAttackRange(TargetActor))
		{
			if (APawn* Pawn = AIController->GetPawn())
			{
				BlackboardComp->SetValueAsVector(ResultKeyName, Pawn->GetActorLocation());
				return EBTNodeResult::Succeeded;
			}
		}
	}
	
	UEnvQueryManager* EnvQueryManager = UEnvQueryManager::GetCurrent(AIController->GetWorld());
	if (!EnvQueryManager) return EBTNodeResult::Failed;

	CachedOwnerComp = &OwnerComp;

	FEnvQueryRequest QueryRequest(QueryTemplate, AIController);
	QueryRequest.SetFloatParam(FName("MinDistance"), MinDist);
	QueryRequest.SetFloatParam(FName("MaxDistance"), MaxDist);

	QueryID = QueryRequest.Execute(
		RunMode,
		FQueryFinishedSignature::CreateUObject(this, &USFBTTask_FindAttackPoint::OnQueryFinished)
	);

	if (QueryID == INDEX_NONE)
	{
		CachedOwnerComp.Reset();
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

// [추가] AbortTask 구현
EBTNodeResult::Type USFBTTask_FindAttackPoint::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (QueryID != INDEX_NONE)
	{
		UEnvQueryManager* EnvQueryManager = UEnvQueryManager::GetCurrent(OwnerComp.GetWorld());
		if (EnvQueryManager)
		{
			EnvQueryManager->AbortQuery(QueryID);
		}
		QueryID = INDEX_NONE;
	}
	
	return Super::AbortTask(OwnerComp, NodeMemory);
}

void USFBTTask_FindAttackPoint::OnQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
    // 이미 태스크가 종료되었거나 Owner가 사라졌으면 무시
	if (!CachedOwnerComp.IsValid()) return;

	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
	QueryID = INDEX_NONE; // ID 초기화

    // 실패 처리 조건들
	if (!Result.IsValid() || !Result->IsSuccessful() || Result->Items.Num() == 0)
	{
		FinishLatentTask(*OwnerComp, FailureResult);
		return;
	}

	FVector ResultLocation = Result->GetItemAsLocation(0);
    AAIController* AIController = OwnerComp->GetAIOwner();

	if (bProjectToNavMesh && AIController)
	{
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(AIController->GetWorld());
		if (NavSys)
		{
			FNavLocation NavLocation;
            // [수정] Z축 범위 500.f로 넉넉하게 확장 (언덕/계단 문제 해결)
			const bool bOnNavMesh = NavSys->ProjectPointToNavigation(
				ResultLocation,
				NavLocation,
				FVector(NavMeshProjectionRadius, NavMeshProjectionRadius, 500.0f)
			);

			if (bOnNavMesh)
			{
				ResultLocation = NavLocation.Location;
			}
			else
			{
				FinishLatentTask(*OwnerComp, FailureResult);
				return;
			}
		}
	}
	
	UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	if (BlackboardComp && ResultKeyName != NAME_None)
	{
		BlackboardComp->SetValueAsVector(ResultKeyName, ResultLocation);
		FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
	}
	else
	{
		FinishLatentTask(*OwnerComp, EBTNodeResult::Failed);
	}
}

void USFBTTask_FindAttackPoint::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (QueryID != INDEX_NONE)
	{
		UEnvQueryManager* EnvQueryManager = UEnvQueryManager::GetCurrent(OwnerComp.GetWorld());
		if (EnvQueryManager)
		{
			EnvQueryManager->AbortQuery(QueryID);
		}
		QueryID = INDEX_NONE;
	}
	
	CachedOwnerComp.Reset();

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}