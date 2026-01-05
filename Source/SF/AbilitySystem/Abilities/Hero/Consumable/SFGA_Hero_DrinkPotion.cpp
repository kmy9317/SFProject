#include "SFGA_Hero_DrinkPotion.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Abilities/SFGameplayAbilityTags.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Item/Fragments/SFItemFragment_Consumable.h"

USFGA_Hero_DrinkPotion::USFGA_Hero_DrinkPotion(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilityTags.AddTag(SFGameplayTags::Ability_Hero_Drink);
	ActivationOwnedTags.AddTag(SFGameplayTags::Character_State_Drink);
}

void USFGA_Hero_DrinkPotion::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!GetConsumeItemInstance())
	{
		return;
	}

	FGameplayTag MontageTag = DefaultDrinkMontageTag;
	if (const USFItemFragment_Consumable* Fragment = GetConsumeFragment())
	{
		if (Fragment->ConsumeMontageTag.IsValid())
		{
			MontageTag = Fragment->ConsumeMontageTag;
		}
	}

	// 몽타주 재생
	if (const USFHeroAnimationData* AnimData = GetHeroAnimationData())
	{
		const FSFMontagePlayData MontageData = AnimData->GetSingleMontage(MontageTag);
		if (MontageData.IsValid())
		{
			UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this,
				TEXT("DrinkMontage"),
				MontageData.Montage,
				MontageData.PlayRate,
				MontageData.StartSection,
				true);

			if (MontageTask)
			{
				MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
				MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageFinished);
				MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageFinished);
				MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageFinished);
				MontageTask->ReadyForActivation();
			}
		}
	}

	// 몽타주 이벤트 대기
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		SFGameplayTags::GameplayEvent_Montage_Begin,
		nullptr,
		true,
		true
	);

	if (EventTask)
	{
		EventTask->EventReceived.AddDynamic(this, &ThisClass::OnDrinkBegin);
		EventTask->ReadyForActivation();
	}
}

void USFGA_Hero_DrinkPotion::OnDrinkBegin(FGameplayEventData Payload)
{
	bItemUsed = true;
	ApplyConsumeEffect();
}

void USFGA_Hero_DrinkPotion::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

