// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGA_EnemyDeath.h"

#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"

USFGA_EnemyDeath::USFGA_EnemyDeath()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bServerRespectsRemoteAbilityCancellation = true;
	
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = SFGameplayTags::GameplayEvent_Death;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

}

void USFGA_EnemyDeath::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (Avatar->HasAuthority())
	{
		FTimerDelegate TimerDel;
		TimerDel.BindUObject(this, &ThisClass::DeathEventAfterDelay);
		Avatar->GetWorldTimerManager().SetTimer(EventTimerHandle, TimerDel, EventTime, false);
	}
}

void USFGA_EnemyDeath::DeathEventAfterDelay()
{
	DestroyEnemy();
}

void USFGA_EnemyDeath::DestroyEnemy()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (Avatar && Avatar->HasAuthority())
	{
		Avatar->Destroy();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}


