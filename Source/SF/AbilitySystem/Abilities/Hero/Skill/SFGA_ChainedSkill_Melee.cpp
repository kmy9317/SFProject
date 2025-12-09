#include "SFGA_ChainedSkill_Melee.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"

USFGA_ChainedSkill_Melee::USFGA_ChainedSkill_Melee(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool USFGA_ChainedSkill_Melee::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (ChainConfigs.Num() == 0)
	{
		return false;
	}
	
	return CanContinueChain();
}

UAbilitySystemComponent* USFGA_ChainedSkill_Melee::GetChainASC() const
{
	return GetAbilitySystemComponentFromActorInfo();
}

void USFGA_ChainedSkill_Melee::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!GetChainASC())
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	ExecutingChainIndex = GetCurrentChain();
}

void USFGA_ChainedSkill_Melee::OnTrace(FGameplayEventData Payload)
{
	Super::OnTrace(Payload);
}

void USFGA_ChainedSkill_Melee::ExecuteChainStep(int32 ChainIndex)
{
	if (!ChainConfigs.IsValidIndex(ChainIndex))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	const FSFChainConfig& ChainConfig = ChainConfigs[ChainIndex];
	CurrentDamageMultiplier = ChainConfig.DamageMultiplier;

	UAnimMontage* MontageToPlay = ChainConfig.Montage;
	if (!MontageToPlay)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	ApplyChainEffects(ChainIndex, this);

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		FName(*FString::Printf(TEXT("ChainStep_%d"), ChainIndex)),
		MontageToPlay,
		1.0f,
		NAME_None,
		true
	);

	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnChainMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnChainMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnChainMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnChainMontageInterrupted);
		MontageTask->ReadyForActivation();
	}

	// Trace
	UAbilityTask_WaitGameplayEvent* TraceTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		SFGameplayTags::GameplayEvent_Tracing,
		nullptr,
		false,
		true
	);
	if (TraceTask)
	{
		TraceTask->EventReceived.AddDynamic(this, &ThisClass::OnTrace);
		TraceTask->ReadyForActivation();
	}
}

void USFGA_ChainedSkill_Melee::OnChainMontageCompleted()
{
	RemoveChainEffects();

	if (IsLastChain(ExecutingChainIndex))
	{
		CompleteCombo(this);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_ChainedSkill_Melee::OnChainMontageInterrupted()
{
	RemoveChainEffects();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_ChainedSkill_Melee::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ExecutingChainIndex = 0;
	CurrentDamageMultiplier = 1.0f;
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


