#include "SFBTTask_FindAttackPoint.h"

#include "AI/Controller/SFBaseAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "NavigationSystem.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"
#include "Character/SFCharacterBase.h"

USFBTTask_FindAttackPoint::USFBTTask_FindAttackPoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = "Find Attack Point";
	bNotifyTaskFinished = true;
	bNotifyTick = false;
	bCreateNodeInstance = true;
}

EBTNodeResult::Type USFBTTask_FindAttackPoint::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	if (!QueryTemplate)
		return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
		return EBTNodeResult::Failed;

	ASFBaseAIController* AICon = Cast<ASFBaseAIController>(OwnerComp.GetAIOwner());
	if (!AICon)
		return EBTNodeResult::Failed;

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetActorKeyName));
	if (!TargetActor)
		return EBTNodeResult::Failed;

	const FName AbilityTagName = BB->GetValueAsName(AbilityTagKeyName);
	if (AbilityTagName.IsNone())
		return EBTNodeResult::Failed;

	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName);
	if (!AbilityTag.IsValid())
		return EBTNodeResult::Failed;

	float MinRange = 0.f;
	float MaxRange = 0.f;

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(AICon->GetPawn());
	if (!Character)
		return EBTNodeResult::Failed;

	USFAbilitySystemComponent* ASC = Character->GetSFAbilitySystemComponent();
	if (!ASC)
		return EBTNodeResult::Failed;

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.Ability)
			continue;

		if (!Spec.Ability->AbilityTags.HasTag(AbilityTag) &&
			!Spec.Ability->GetAssetTags().HasTag(AbilityTag))
			continue;

		UGameplayAbility* AbilityInstance = Spec.GetPrimaryInstance();
		if (!AbilityInstance)
			AbilityInstance = Spec.Ability;

		if (const USFGA_Enemy_BaseAttack* BaseAttack =
			Cast<USFGA_Enemy_BaseAttack>(AbilityInstance))
		{
			MinRange = BaseAttack->GetMinAttackRange();
			MaxRange = BaseAttack->GetAttackRange();
			break;
		}
	}

	if (MaxRange <= 0.f)
		return EBTNodeResult::Failed;

	if (bSkipIfInRange)
	{
		const float Dist = FVector::Dist(
			Character->GetActorLocation(),
			TargetActor->GetActorLocation());

		if (Dist <= MaxRange && Dist > MinRange)
		{
			BB->SetValueAsVector(ResultKeyName, Character->GetActorLocation());
			return EBTNodeResult::Succeeded;
		}
	}

	const float OptimalDistance = (MinRange + MaxRange) * 0.5f;

	UEnvQueryManager* EQS = UEnvQueryManager::GetCurrent(AICon->GetWorld());
	if (!EQS)
		return EBTNodeResult::Failed;

	CachedOwnerComp = &OwnerComp;

	FEnvQueryRequest QueryRequest(QueryTemplate, AICon);
	QueryRequest.SetFloatParam(TEXT("MinDistance"), MinRange);
	QueryRequest.SetFloatParam(TEXT("MaxDistance"), MaxRange);
	QueryRequest.SetFloatParam(TEXT("OptimalDistance"), OptimalDistance);

	QueryID = QueryRequest.Execute(
		RunMode,
		FQueryFinishedSignature::CreateUObject(
			this,
			&USFBTTask_FindAttackPoint::OnQueryFinished));

	if (QueryID == INDEX_NONE)
	{
		CachedOwnerComp.Reset();
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type USFBTTask_FindAttackPoint::AbortTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	if (QueryID != INDEX_NONE)
	{
		if (UEnvQueryManager* EQS = UEnvQueryManager::GetCurrent(OwnerComp.GetWorld()))
		{
			EQS->AbortQuery(QueryID);
		}
		QueryID = INDEX_NONE;
	}

	return Super::AbortTask(OwnerComp, NodeMemory);
}

void USFBTTask_FindAttackPoint::OnQueryFinished(
	TSharedPtr<FEnvQueryResult> Result)
{
	if (!CachedOwnerComp.IsValid())
		return;

	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
	QueryID = INDEX_NONE;

	if (!Result.IsValid() || !Result->IsSuccessful() || Result->Items.IsEmpty())
	{
		FinishLatentTask(*OwnerComp, FailureResult);
		return;
	}

	FVector Location = Result->GetItemAsLocation(0);

	if (bProjectToNavMesh)
	{
		AAIController* AICon = OwnerComp->GetAIOwner();
		if (!AICon)
		{
			FinishLatentTask(*OwnerComp, FailureResult);
			return;
		}

		if (UNavigationSystemV1* NavSys =
			UNavigationSystemV1::GetCurrent(AICon->GetWorld()))
		{
			FNavLocation NavLoc;
			if (NavSys->ProjectPointToNavigation(
				Location,
				NavLoc,
				FVector(NavMeshProjectionRadius)))
			{
				Location = NavLoc.Location;
			}
			else
			{
				FinishLatentTask(*OwnerComp, FailureResult);
				return;
			}
		}
	}

	if (UBlackboardComponent* BB = OwnerComp->GetBlackboardComponent())
	{
		BB->SetValueAsVector(ResultKeyName, Location);
		FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
	}
	else
	{
		FinishLatentTask(*OwnerComp, EBTNodeResult::Failed);
	}
}

void USFBTTask_FindAttackPoint::OnTaskFinished(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	EBTNodeResult::Type TaskResult)
{
	if (QueryID != INDEX_NONE)
	{
		if (UEnvQueryManager* EQS = UEnvQueryManager::GetCurrent(OwnerComp.GetWorld()))
		{
			EQS->AbortQuery(QueryID);
		}
		QueryID = INDEX_NONE;
	}

	CachedOwnerComp.Reset();
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
