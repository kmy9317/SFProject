#include "SFAbilitySystemLibrary.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterGameplayTags.h"

void USFAbilitySystemLibrary::SendGameplayEventFromSpec(UAbilitySystemComponent* ASC, const FGameplayTag& EventTag, const FGameplayEffectSpec& Spec)
{
	if (!ASC)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Target = ASC->GetAvatarActor();
	Payload.Instigator = Spec.GetContext().GetOriginalInstigator();
	Payload.ContextHandle = Spec.GetContext();
	ASC->HandleGameplayEvent(EventTag, &Payload);
}

void USFAbilitySystemLibrary::SendDeathEventFromSpec(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec)
{
	if (!ASC)
	{
		return;
	}

	if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
	{
		return;
	}

	SendGameplayEventFromSpec(ASC, SFGameplayTags::GameplayEvent_Death, Spec);
}

void USFAbilitySystemLibrary::SendDownedEventFromSpec(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec)
{
	if (!ASC)
	{
		return;
	}

	if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
	{
		return;
	}

	if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Downed))
	{
		return;
	}

	SendGameplayEventFromSpec(ASC, SFGameplayTags::GameplayEvent_Downed, Spec);
}

void USFAbilitySystemLibrary::SendHitReactionEventFromSpec(UAbilitySystemComponent* ASC, float Damage, const FGameplayEffectSpec& Spec)
{
	if (!ASC)
	{
		return;
	}

	if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
	{
		return;
	}

	if (Damage <= 1.f)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = SFGameplayTags::GameplayEvent_HitReaction;
	Payload.Target = ASC->GetAvatarActor();
	Payload.Instigator = Spec.GetContext().GetOriginalInstigator();
	Payload.ContextHandle = Spec.GetContext();
	Payload.EventMagnitude = Damage;
	ASC->HandleGameplayEvent(SFGameplayTags::GameplayEvent_HitReaction, &Payload);
}

void USFAbilitySystemLibrary::SendParryEventFromSpec(UAbilitySystemComponent* ASC, float Damage, const FGameplayEffectSpec& Spec)
{
	if (!ASC)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = SFGameplayTags::GameplayEvent_Parry;
	Payload.Target = ASC->GetAvatarActor();
	Payload.Instigator = Spec.GetContext().GetOriginalInstigator();
	Payload.ContextHandle = Spec.GetContext();
	Payload.EventMagnitude = Damage;
	ASC->HandleGameplayEvent(SFGameplayTags::GameplayEvent_Parry, &Payload);
}

void USFAbilitySystemLibrary::SendStaggerEventFromSpec(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec)
{
	if (!ASC)
	{
		return;
	}

	if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
	{
		return;
	}

	SendGameplayEventFromSpec(ASC, SFGameplayTags::GameplayEvent_Groggy, Spec);
}

void USFAbilitySystemLibrary::SendGameplayEvent(UAbilitySystemComponent* ASC, const FGameplayTag& EventTag, AActor* Instigator)
{
	if (!ASC)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Target = ASC->GetAvatarActor();
	Payload.Instigator = Instigator;
	ASC->HandleGameplayEvent(EventTag, &Payload);
}

void USFAbilitySystemLibrary::SendDeathEvent(UAbilitySystemComponent* ASC, AActor* Instigator)
{
	if (!ASC)
	{
		return;
	}

	if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
	{
		return;
	}

	SendGameplayEvent(ASC, SFGameplayTags::GameplayEvent_Death, Instigator);
}
