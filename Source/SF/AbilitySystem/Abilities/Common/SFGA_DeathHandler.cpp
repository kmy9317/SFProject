#include "SFGA_DeathHandler.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Abilities/Hero/Skill/SFHeroSkillTags.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Libraries/SFAbilitySystemLibrary.h"
#include "Player/Components/SFPlayerCombatStateComponent.h"

USFGA_DeathHandler::USFGA_DeathHandler(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = ESFAbilityActivationPolicy::OnSpawn;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void USFGA_DeathHandler::ActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilityTask_WaitGameplayEvent* WaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		SFGameplayTags::GameplayEvent_ZeroHealth,
		nullptr,
		false,
		true  // OnlyMatchExact
	);

	if (WaitTask)
	{
		WaitTask->EventReceived.AddDynamic(this, &USFGA_DeathHandler::OnZeroHealthReceived);
		WaitTask->ReadyForActivation();
	}
}

void USFGA_DeathHandler::OnZeroHealthReceived(FGameplayEventData Payload)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// 이미 Dead 상태면 무시
	if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
	{
		return;
	}

	// 1순위: (LastStand 체크)
	if (TryLastStand(ASC, Payload))
	{
		return;
	}

	// 2순위: (Downed 상태 체크)
	if (CanEnterDownedState())
	{
		USFAbilitySystemLibrary::SendDownedEvent(ASC, Payload);
		return;
	}

	// 3순위: 최종 사망
	USFAbilitySystemLibrary::SendDeathEvent(ASC, Payload);
}

bool USFGA_DeathHandler::TryLastStand(UAbilitySystemComponent* ASC, const FGameplayEventData& Payload)
{
	const FGameplayTag LastStandAvailableTag = SFGameplayTags::Ability_Skill_Passive_LastStand;
	const FGameplayTag LastStandUsedTag = SFGameplayTags::Ability_Skill_Passive_LastStand_Use;
	if (!ASC->HasMatchingGameplayTag(LastStandAvailableTag))
	{
		return false;
	}

	if (ASC->HasMatchingGameplayTag(LastStandUsedTag))
	{
		return false;
	}

	// 기존 LastStand GA에 이벤트 전달
	FGameplayEventData LastStandEvent;
	LastStandEvent.EventTag = SFGameplayTags::GameplayEvent_PlayerAbility_LastStand;
	LastStandEvent.Instigator = Payload.Instigator;
	LastStandEvent.Target = Payload.Target;
	LastStandEvent.ContextHandle = Payload.ContextHandle;
	ASC->HandleGameplayEvent(LastStandEvent.EventTag, &LastStandEvent);
	return true;
}

bool USFGA_DeathHandler::CanEnterDownedState() const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return false;
	}

	USFPlayerCombatStateComponent* CombatState = USFPlayerCombatStateComponent::FindPlayerCombatStateComponent(AvatarActor);
	if (!CombatState)
	{
		return false;
	}

	return CombatState->GetRemainingDownCount() > 0;
}