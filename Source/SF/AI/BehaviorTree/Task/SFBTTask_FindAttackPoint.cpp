// Fill out your copyright notice in the Description page of Project Settings.

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
}

EBTNodeResult::Type USFBTTask_FindAttackPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{

	if (!QueryTemplate)
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	ASFEnemyController* AIController = Cast<ASFEnemyController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}


	FName SelectedAbilityTagName = BlackboardComp->GetValueAsName(AbilityTagKeyName);
	if (SelectedAbilityTagName == NAME_None)
	{
		return EBTNodeResult::Failed;
	}

	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKeyName));
	if (!TargetActor)
	{
		return EBTNodeResult::Failed;
	}
	
	FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(SelectedAbilityTagName);
	if (!AbilityTag.IsValid())
	{
		return EBTNodeResult::Failed;
	}
	
	float MinDist = 0.0f;
	float MaxDist = 1000.0f;
	USFGA_Enemy_BaseAttack* BaseAttack = nullptr;

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(AIController->GetPawn());
	if (Character)
	{
		USFAbilitySystemComponent* ASC = Character->GetSFAbilitySystemComponent();
		if (ASC)
		{
			// Ability 찾기
			for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
			{
				if (Spec.Ability)
				{
					
					bool bTagMatches = false;
					
					for (const FGameplayTag& SpecAbilityTag : Spec.Ability->AbilityTags)
					{
						if (SpecAbilityTag.MatchesTag(AbilityTag))
						{
							bTagMatches = true;
							break;
						}
					}
					
					if (!bTagMatches && Spec.Ability->AbilityTags.Num() == 0)
					{
						for (const FGameplayTag& AssetTag : Spec.Ability->GetAssetTags())
						{
							if (AssetTag.MatchesTag(AbilityTag))
							{
								bTagMatches = true;
								break;
							}
						}
					}

					if (bTagMatches)
					{
						BaseAttack = Cast<USFGA_Enemy_BaseAttack>(Spec.Ability);
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
	if (!EnvQueryManager)
	{
		return EBTNodeResult::Failed;
	}

	CachedOwnerComp = &OwnerComp;

	// EQS Query 요청 생성
	FEnvQueryRequest QueryRequest(QueryTemplate, AIController);
	QueryRequest.SetFloatParam(FName("MinDistance"), MinDist);
	QueryRequest.SetFloatParam(FName("MaxDistance"), MaxDist);

	// 쿼리 실행
	QueryID = QueryRequest.Execute(
		RunMode,
		FQueryFinishedSignature::CreateUObject(
			this,
			&USFBTTask_FindAttackPoint::OnQueryFinished
		)
	);

	if (QueryID == INDEX_NONE)
	{
		CachedOwnerComp.Reset();
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

void USFBTTask_FindAttackPoint::OnQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
	if (!OwnerComp)
	{
		return;
	}

	AAIController* AIController = OwnerComp->GetAIOwner();
	EBTNodeResult::Type TaskResult = FailureResult;

	if (!Result.IsValid())
	{
		FinishLatentTask(*OwnerComp, TaskResult);
		return;
	}

	if (!Result->IsSuccessful())
	{
		FinishLatentTask(*OwnerComp, TaskResult);
		return;
	}

	if (Result->Items.Num() == 0)
	{
		FinishLatentTask(*OwnerComp, TaskResult);
		return;
	}


	FVector ResultLocation = Result->GetItemAsLocation(0);


	if (bProjectToNavMesh && AIController)
	{
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(AIController->GetWorld());
		if (NavSys)
		{
			FNavLocation NavLocation;
			const bool bOnNavMesh = NavSys->ProjectPointToNavigation(
				ResultLocation,
				NavLocation,
				FVector(NavMeshProjectionRadius, NavMeshProjectionRadius, NavMeshProjectionRadius)
			);

			if (bOnNavMesh)
			{
				ResultLocation = NavLocation.Location;
			}
			else
			{
				// NavMesh에 없으면 실패
				FinishLatentTask(*OwnerComp, TaskResult);
				return;
			}
		}
	}

	
	UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	if (BlackboardComp && ResultKeyName != NAME_None)
	{
		BlackboardComp->SetValueAsVector(ResultKeyName, ResultLocation);
		TaskResult = EBTNodeResult::Succeeded;
	}
	else
	{
		TaskResult = EBTNodeResult::Failed;
	}

	FinishLatentTask(*OwnerComp, TaskResult);
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

