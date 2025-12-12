// Fill out your copyright notice in the Description page of Project Settings.
#include "SFDragonCombatComponent.h"
#include "AbilitySystemGlobals.h"
#include "AIController.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Enemy/SFCombatSet_Enemy.h"
#include "AbilitySystem/Attributes/Enemy/SFPrimarySet_Enemy.h"
#include "Character/SFCharacterBase.h"



USFDragonCombatComponent::USFDragonCombatComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) 
{
	PrimaryComponentTick.bCanEverTick = false;
}



void USFDragonCombatComponent::InitializeCombatComponent()
{
	AAIController* Controller = GetController<AAIController>();
	if (!Controller) return;
	
	CachedASC = Cast<USFAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Controller->GetPawn()));
	
	if (!CachedASC)
	{
		return;
	}
	
	const USFPrimarySet_Enemy* PrimarySet = CachedASC->GetSet<USFPrimarySet_Enemy>();
	if (PrimarySet)
	{
		USFPrimarySet_Enemy* Set = const_cast<USFPrimarySet_Enemy*>(PrimarySet);
		Set->OnTakeDamageDelegate.RemoveDynamic(this, &ThisClass::AddThreat);
		Set->OnTakeDamageDelegate.AddDynamic(this, &ThisClass::AddThreat);
	}
	
}

void USFDragonCombatComponent::AddThreat( float ThreatValue, AActor* Actor)
{
	if (ThreatMap.Contains(Actor))
	{
		ThreatMap[Actor] += ThreatValue;
	}
}

AActor* USFDragonCombatComponent::GetHighestThreatActor()
{
	if (ThreatMap.Num()> 0)
	{
		AActor* HighestThreatActor = nullptr;
		float HighestThreatValue = 0.f;
		for (auto& ThreatPair : ThreatMap)
		{
			if (HighestThreatActor == nullptr)
			{
				HighestThreatActor = ThreatPair.Key;
				HighestThreatValue = ThreatPair.Value;
			}
			else if(ThreatPair.Value > HighestThreatValue)
			{
				HighestThreatValue = ThreatPair.Value;
				HighestThreatActor = ThreatPair.Key;
			}
		}
		return HighestThreatActor;
	}
	return nullptr;
}


