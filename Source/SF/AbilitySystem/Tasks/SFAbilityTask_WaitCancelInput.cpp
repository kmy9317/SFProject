#include "SFAbilityTask_WaitCancelInput.h"
#include "AbilitySystemComponent.h"

USFAbilityTask_WaitCancelInput::USFAbilityTask_WaitCancelInput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

USFAbilityTask_WaitCancelInput* USFAbilityTask_WaitCancelInput::WaitCancelInput(UGameplayAbility* OwningAbility)
{
	return NewAbilityTask<USFAbilityTask_WaitCancelInput>(OwningAbility);
}

void USFAbilityTask_WaitCancelInput::Activate()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC && Ability)
	{
		DelegateHandle = ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::GameCustom2,GetAbilitySpecHandle(),GetActivationPredictionKey()).AddUObject(this, &USFAbilityTask_WaitCancelInput::OnCancelCallback);

		if (IsForRemoteClient())
		{
			if (!ASC->CallReplicatedEventDelegateIfSet(EAbilityGenericReplicatedEvent::GameCustom2,GetAbilitySpecHandle(),GetActivationPredictionKey()))
			{
				SetWaitingOnRemotePlayerData();
			}
		}
	}
}

void USFAbilityTask_WaitCancelInput::OnCancelCallback()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!Ability || !ASC)
	{
		return;
	}

	ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::GameCustom2,GetAbilitySpecHandle(),GetActivationPredictionKey()).Remove(DelegateHandle);

	FScopedPredictionWindow ScopedPrediction(ASC, IsPredictingClient());
	if (IsPredictingClient())
	{
		ASC->ServerSetReplicatedEvent(EAbilityGenericReplicatedEvent::GameCustom2,GetAbilitySpecHandle(),GetActivationPredictionKey(),ASC->ScopedPredictionKey);
	}
	else
	{
		ASC->ConsumeGenericReplicatedEvent(EAbilityGenericReplicatedEvent::GameCustom2,GetAbilitySpecHandle(),GetActivationPredictionKey());
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCancelInput.Broadcast();
	}

	EndTask();
}