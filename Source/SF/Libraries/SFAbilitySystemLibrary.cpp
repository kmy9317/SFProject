#include "SFAbilitySystemLibrary.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Character/SFCharacterGameplayTags.h"

namespace
{
	FGameplayEventData MakePayloadFromSpec(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, float Damage = 0.0f)
	{
		FGameplayEventData Payload;
		Payload.Target = ASC->GetAvatarActor();
		Payload.Instigator = Spec.GetContext().GetOriginalInstigator();
		Payload.ContextHandle = Spec.GetContext();
		Payload.EventMagnitude = Damage;
		return Payload;
	}
	
	void DispatchGameplayEvent(UAbilitySystemComponent* ASC, const FGameplayTag& EventTag, const FGameplayEventData& Source)
	{
		FGameplayEventData EventData;
		EventData.EventTag = EventTag;
		EventData.Target = Source.Target;
		EventData.Instigator = Source.Instigator;
		EventData.ContextHandle = Source.ContextHandle;
		EventData.EventMagnitude = Source.EventMagnitude;

		ASC->HandleGameplayEvent(EventTag, &EventData);
	}
}

void USFAbilitySystemLibrary::SendGameplayEventFromSpec(UAbilitySystemComponent* ASC, const FGameplayTag& EventTag, const FGameplayEffectSpec& Spec)
{
	if (!ASC)
	{
		return;
	}

	FGameplayEventData Payload = MakePayloadFromSpec(ASC, Spec);
	DispatchGameplayEvent(ASC, EventTag, Payload);
}

void USFAbilitySystemLibrary::SendZeroHealthEventFromSpec(UAbilitySystemComponent* ASC, float Damage, const FGameplayEffectSpec& Spec)
{
	if (!ASC)
	{
		return;
	}

	// Dead 상태에서 중복 발송 방지
	if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
	{
		return;
	}

	FGameplayEventData Payload = MakePayloadFromSpec(ASC, Spec, Damage);
	DispatchGameplayEvent(ASC, SFGameplayTags::GameplayEvent_ZeroHealth, Payload);
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

	FGameplayEventData Payload = MakePayloadFromSpec(ASC, Spec, Damage);
	DispatchGameplayEvent(ASC, SFGameplayTags::GameplayEvent_HitReaction, Payload);
}

void USFAbilitySystemLibrary::SendParryEventFromSpec(UAbilitySystemComponent* ASC, float Damage, const FGameplayEffectSpec& Spec)
{
	if (!ASC)
	{
		return;
	}

	FGameplayEventData Payload = MakePayloadFromSpec(ASC, Spec, Damage);
	DispatchGameplayEvent(ASC, SFGameplayTags::GameplayEvent_Parry, Payload);
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

	FGameplayEventData Payload = MakePayloadFromSpec(ASC, Spec);
	DispatchGameplayEvent(ASC, SFGameplayTags::GameplayEvent_Groggy, Payload);
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

void USFAbilitySystemLibrary::SendDeathEvent(UAbilitySystemComponent* ASC, const FGameplayEventData& SourcePayload)
{
	if (!ASC)
	{
		return;
	}

	if (ASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Dead))
	{
		return;
	}

	DispatchGameplayEvent(ASC, SFGameplayTags::GameplayEvent_Death, SourcePayload);
}

void USFAbilitySystemLibrary::SendDownedEvent(UAbilitySystemComponent* ASC, const FGameplayEventData& SourcePayload)
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

	DispatchGameplayEvent(ASC, SFGameplayTags::GameplayEvent_Downed, SourcePayload);
}
