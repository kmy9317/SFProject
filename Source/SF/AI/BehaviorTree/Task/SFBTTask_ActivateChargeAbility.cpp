// Fill out your copyright notice in the Description page of Project Settings.

#include "SFBTTask_ActivateChargeAbility.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/SFCharacterGameplayTags.h"

USFBTTask_ActivateAbilityByTag::USFBTTask_ActivateAbilityByTag(
	const FObjectInitializer& ObjectInitializer
): Super(ObjectInitializer)
{
	NodeName = "Activate Ability By Tag ";
	bCreateNodeInstance = true;
	bNotifyTaskFinished = true;
}

UAbilitySystemComponent* USFBTTask_ActivateAbilityByTag::GetASC(
	UBehaviorTreeComponent& OwnerComp
) const
{
	if (AAIController* AI = OwnerComp.GetAIOwner())
	{
		return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(
			AI->GetPawn()
		);
	}
	return nullptr;
}

EBTNodeResult::Type USFBTTask_ActivateAbilityByTag::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory
)
{
	if (!AbilityTag.IsValid())
		return EBTNodeResult::Failed;

	UAbilitySystemComponent* ASC = GetASC(OwnerComp);
	if (!ASC)
		return EBTNodeResult::Failed;

	CachedASC = ASC;
	CachedOwnerComp = &OwnerComp;
	
	TArray<FGameplayAbilitySpec*> Abilities;
	FGameplayTagContainer Tags;
	Tags.AddTag(AbilityTag);

	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		Tags, Abilities, false
	);

	if (Abilities.Num() == 0)
	{
		return EBTNodeResult::Failed;
	}
	
	FGameplayAbilitySpec* Spec = Abilities[0];
	if (!ASC->TryActivateAbility(Spec->Handle))
	{
		return EBTNodeResult::Failed;
	}
	
	ExecutingAbilityHandle = Spec->Handle;

	// Ability 종료 감시
	AbilityEndedHandle = ASC->OnAbilityEnded.AddUObject(
			this,
			&USFBTTask_ActivateAbilityByTag::OnAbilityEnded
		);

	return EBTNodeResult::InProgress;
}

void USFBTTask_ActivateAbilityByTag::OnAbilityEnded(
	const FAbilityEndedData& EndedData
)
{
	// 우리가 실행한 Ability만 반응
	if (EndedData.AbilitySpecHandle != ExecutingAbilityHandle)
		return;

	if (!CachedOwnerComp.IsValid())
		return;

	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();

	if (EndedData.bWasCancelled)
	{
		FinishLatentTask(*OwnerComp, EBTNodeResult::Failed);
	}
	else
	{
		FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
	}
}

void USFBTTask_ActivateAbilityByTag::OnTaskFinished(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	EBTNodeResult::Type TaskResult
)
{
	if (CachedASC.IsValid() && AbilityEndedHandle.IsValid())
	{
		CachedASC->OnAbilityEnded.Remove(AbilityEndedHandle);
	}

	ExecutingAbilityHandle = FGameplayAbilitySpecHandle();
	CachedASC.Reset();
	CachedOwnerComp.Reset();

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void USFBTTask_ActivateAbilityByTag::CleanupDelegates(UBehaviorTreeComponent& OwnerComp)
{
	if (CachedASC.IsValid() && AbilityEndedHandle.IsValid())
	{
		CachedASC->OnAbilityEnded.Remove(AbilityEndedHandle);
		AbilityEndedHandle.Reset();
	}
}

EBTNodeResult::Type USFBTTask_ActivateAbilityByTag::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UAbilitySystemComponent* ASC = GetASC(OwnerComp);
	if (ASC && ExecutingAbilityHandle.IsValid())
	{
		ASC->CancelAbilityHandle(ExecutingAbilityHandle);
	}

	CleanupDelegates(OwnerComp);

	return EBTNodeResult::Aborted;
}
