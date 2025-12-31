// Fill out your copyright notice in the Description page of Project Settings.


#include "SFBTTask_SelectAbilityByTag.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AIController.h"
#include "AbilitySystem/Abilities/Enemy/Combat/SFGA_Enemy_BaseAttack.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Interface/SFAIControllerInterface.h"
#include "Interface/SFEnemyAbilityInterface.h"

USFBTTask_SelectAbilityByTag::USFBTTask_SelectAbilityByTag()
{
	NodeName = TEXT("Select Ability(Task)");

	MinRangeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(USFBTTask_SelectAbilityByTag, MinRangeKey));
	MaxRangeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(USFBTTask_SelectAbilityByTag, MaxRangeKey));
	
}

EBTNodeResult::Type USFBTTask_SelectAbilityByTag::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	APawn* Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	if (!Pawn) return EBTNodeResult::Failed;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
	if (!ASC) return EBTNodeResult::Failed;


	if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_UsingAbility))
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;


	FName SelectedTagName = BB->GetValueAsName(BlackboardKey.SelectedKeyName);
	if (!SelectedTagName.IsNone())
	{
		return EBTNodeResult::Succeeded;
	}


	if (ISFAIControllerInterface* AI = Cast<ISFAIControllerInterface>(OwnerComp.GetAIOwner()))
	{
		USFCombatComponentBase* Combat = AI->GetCombatComponent();
		if (!Combat) return EBTNodeResult::Failed;

		FEnemyAbilitySelectContext Context;
		Context.Self = Pawn;
		Context.Target = Combat->GetCurrentTarget();

		FGameplayTag OutSelectedTag;
		if (Combat->SelectAbility(Context, AbilitySearchTags, OutSelectedTag))
		{
			
			BB->SetValueAsName(BlackboardKey.SelectedKeyName, OutSelectedTag.GetTagName());
			
			
			for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
			{
				if (Spec.Ability && Spec.Ability->AbilityTags.HasTagExact(OutSelectedTag))
				{
					
					const float* MinValPtr = Spec.SetByCallerTagMagnitudes.Find(SFGameplayTags::Data_EnemyAbility_MinAttackRange);
					const float* MaxValPtr = Spec.SetByCallerTagMagnitudes.Find(SFGameplayTags::Data_EnemyAbility_AttackRange);

			
					float MinRange = MinValPtr ? *MinValPtr : 0.f;
					float MaxRange = MaxValPtr ? *MaxValPtr : 200.f; 

			
					if (MaxRange <= 0.f) MaxRange = 999999.f;

			
					BB->SetValueAsFloat(MinRangeKey.SelectedKeyName, MinRange);
					BB->SetValueAsFloat(MaxRangeKey.SelectedKeyName, MaxRange);
					break; 
				}
			}
			return EBTNodeResult::Succeeded;
		}
		else
		{
			BB->ClearValue(BlackboardKey.SelectedKeyName);
		}
	}

	return EBTNodeResult::Failed;
}
