#include "SFGA_Thrust_DivineSlash.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"


USFGA_Thrust_DivineSlash::USFGA_Thrust_DivineSlash(const FObjectInitializer& ObjectInitializer)
{
	
}

void USFGA_Thrust_DivineSlash::ExecuteChainStep(int32 ChainIndex)
{
	// UAbilityTask_WaitGameplayEvent* InvincStartTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
	// 	this,
	// 	SFGameplayTags::GameplayEvent_Invincibility_Start,
	// 	nullptr,
	// 	true,
	// 	true
	// );
	//
	// if (InvincStartTask)
	// {
	// 	InvincStartTask->EventReceived.AddDynamic(this, &ThisClass::OnInvincibilityStart);
	// 	InvincStartTask->ReadyForActivation();
	// }
	//
	// // 무적 종료 이벤트 대기
	// UAbilityTask_WaitGameplayEvent* InvincEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
	// 	this,
	// 	SFGameplayTags::GameplayEvent_Invincibility_End,
	// 	nullptr,
	// 	true,
	// 	true
	// );
	//
	// if (InvincEndTask)
	// {
	// 	InvincEndTask->EventReceived.AddDynamic(this, &ThisClass::OnInvincibilityEnd);
	// 	InvincEndTask->ReadyForActivation();
	// }
	
	
	// 부모의 공격 실행
	Super::ExecuteChainStep(ChainIndex);
}

void USFGA_Thrust_DivineSlash::OnChainMontageCompleted()
{
	RemoveInvincibility();
	Super::OnChainMontageCompleted();
}

void USFGA_Thrust_DivineSlash::OnChainMontageInterrupted()
{
	RemoveInvincibility();
	Super::OnChainMontageInterrupted();
}

void USFGA_Thrust_DivineSlash::OnInvincibilityStart(FGameplayEventData Payload)
{
	ApplyInvincibility();
}

void USFGA_Thrust_DivineSlash::OnInvincibilityEnd(FGameplayEventData Payload)
{
	RemoveInvincibility();
}

void USFGA_Thrust_DivineSlash::ApplyInvincibility()
{
	if (!InvincibilityEffectClass || InvincibilityEffectHandle.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetChainASC();
	if (!ASC)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(InvincibilityEffectClass);
	if (SpecHandle.IsValid())
	{
		InvincibilityEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void USFGA_Thrust_DivineSlash::RemoveInvincibility()
{
	if (!InvincibilityEffectHandle.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetChainASC();
	if (ASC)
	{
		ASC->RemoveActiveGameplayEffect(InvincibilityEffectHandle);
	}
	InvincibilityEffectHandle.Invalidate();
}
