// Fill out your copyright notice in the Description page of Project Settings.


#include "BossPhaseState.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "AbilitySystem/Abilities/Enemy/SFEnemyAbilityInitializer.h"
#include "AI/StateMachine/SFStateMachine.h"
#include "System/SFGameInstance.h"

void UBossPhaseState::OnEnter_Implementation()
{
	Super::OnEnter_Implementation();
	if (OwnerActor->HasAuthority())
	{
		if (StateMachine && BehaviourTag.IsValid())
		{   // BT 장창
			if (StateMachine->OnChangeTreeDelegate.IsBound())
			{
				StateMachine->OnChangeTreeDelegate.Broadcast(BehaviourTag);
			}
		}

		if (PhaseAbilities.Num() > 0)
		{
			GivePhaseAbilities();
		}
	}
	
}

void UBossPhaseState::OnExit_Implementation()
{
	Super::OnExit_Implementation();
	if (OwnerActor->HasAuthority())
	{
		ClearPhaseAbilities();
	}
}

void UBossPhaseState::GivePhaseAbilities()
{
	if (OwnerActor->HasAuthority())
	{
		USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor));
		if (!ASC) return;

		for (const TSubclassOf<USFGameplayAbility>& AbilityClass : PhaseAbilities)
		{
			if (!AbilityClass) continue;

			FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, OwnerActor);
			FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);

			USFGameInstance* GI = Cast<USFGameInstance>(OwnerActor->GetWorld()->GetGameInstance());
			if (GI && Handle.IsValid())
			{
				USFGameplayAbility* CDO = Cast<USFGameplayAbility>(AbilityClass->GetDefaultObject());
				if (CDO)
				{
					FName AbilityID = CDO->GetAbilityID();
					if (!AbilityID.IsNone())
					{
						if (const FAbilityBaseData* Data = GI->FindAbilityData(AbilityID))
						{
							FGameplayAbilitySpec* RealSpec = ASC->FindAbilitySpecFromHandle(Handle);
							if (RealSpec)
							{
								USFEnemyAbilityInitializer::ApplyAbilityData(*RealSpec, *Data);
							}
						}
					}
				}
				
			}
			GrantedPhaseAbilityHandles.Add(Handle);
		}	
	}
	
}

void UBossPhaseState::ClearPhaseAbilities()
{
	if (!OwnerActor->HasAuthority()) return;

	USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor));

	if (!ASC) return;

	for (const FGameplayAbilitySpecHandle& Handle : GrantedPhaseAbilityHandles)
	{
		ASC->ClearAbility(Handle);
	}                                                                                                                         

	GrantedPhaseAbilityHandles.Empty();
		
}
