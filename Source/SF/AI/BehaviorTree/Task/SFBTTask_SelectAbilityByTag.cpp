// Fill out your copyright notice in the Description page of Project Settings.


#include "SFBTTask_SelectAbilityByTag.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AIController.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AI/Controller/SFEnemyCombatComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Interface/SFAIControllerInterface.h"
#include "Interface/SFEnemyAbilityInterface.h"

USFBTTask_SelectAbilityByTag::USFBTTask_SelectAbilityByTag()
{
	NodeName = TEXT("Select Ability(Task)");
}

EBTNodeResult::Type USFBTTask_SelectAbilityByTag::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
	if (!ASC)
	{
		return EBTNodeResult::Failed;
	}
	
	if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_UsingAbility))
	{
		return EBTNodeResult::Failed;
	}

	if (ISFAIControllerInterface* AI = Cast<ISFAIControllerInterface>(AIController))
	{
		USFEnemyCombatComponent* Combat = AI->GetCombatComponent();
		if (!Combat)
		{
			return EBTNodeResult::Failed;
		}

		FEnemyAbilitySelectContext Context;
		Context.Self = Pawn;
		Context.Target = Combat->GetCurrentTarget();

		FGameplayTag SelectedTag;
		if (Combat->SelectAbility(Context, AbilitySearchTags, SelectedTag))
		{
			OwnerComp.GetBlackboardComponent()->SetValueAsName(
				BlackboardKey.SelectedKeyName,
				SelectedTag.GetTagName()
			);

			return EBTNodeResult::Succeeded;
		}
		else
		{
			OwnerComp.GetBlackboardComponent()->ClearValue(BlackboardKey.SelectedKeyName);
		}
	}

	return EBTNodeResult::Failed;
}

