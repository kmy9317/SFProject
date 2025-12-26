#include "AbilitySystem/Abilities/Hero/Skill/SFGA_Hero_LastStand.h"

#include "AbilitySystemComponent.h"

USFGA_Hero_LastStand::USFGA_Hero_LastStand()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void USFGA_Hero_LastStand::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 스테이지당 1회 사용 태그(있으면 발동 안되게 SFPrimarySet에서 설정)
	ASC->AddLooseGameplayTag(
		FGameplayTag::RequestGameplayTag(
			TEXT("Ability.Skill.Passive.LastStand.Use")));

	// 체력 회복 이펙트
	if (HealEffect)
	{
		ApplyGameplayEffectToOwner(
			Handle, ActorInfo, ActivationInfo,
			HealEffect.GetDefaultObject(), 1);
	}

	// 무적 이펙트
	if (InvincibleEffect)
	{
		ApplyGameplayEffectToOwner(
			Handle, ActorInfo, ActivationInfo,
			InvincibleEffect.GetDefaultObject(), 1);
	}

	// 공격력 증가 이펙트
	if (DamageBoostEffect)
	{
		ApplyGameplayEffectToOwner(
			Handle, ActorInfo, ActivationInfo,
			DamageBoostEffect.GetDefaultObject(), 1);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
