// Fill out your copyright notice in the Description page of Project Settings.


#include "BTD_CheckSpecialAttack.h"

#include "AIController.h"
#include "AI/BehaviorTree/Decorators/Ability/BTD_CanActivateAbilityByTag.h"
#include "Character/SFCharacterBase.h"

UBTD_CheckSpecialAttack::UBTD_CheckSpecialAttack()
{
	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = false;
	bNotifyTick = false;
}

void UBTD_CheckSpecialAttack::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTMemoryInit::Type InitType) const
{
	FBTCanActivateAbilityMemory* Memory = CastInstanceNodeMemory<FBTCanActivateAbilityMemory>(NodeMemory);
	Memory->bLastResult = false;
	Memory->TimeSinceLastCheck = 0.f;
}

bool UBTD_CheckSpecialAttack::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return false;
	}
	const ASFCharacterBase* SFCharacter = Cast<ASFCharacterBase>(AIController->GetPawn());
	if (!SFCharacter)
	{
		return false;
	}

	USFAbilitySystemComponent* ASC = SFCharacter->GetSFAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	return true;
	
	
}



