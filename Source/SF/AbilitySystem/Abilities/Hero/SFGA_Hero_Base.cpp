// Fill out your copyright notice in the Description page of Project Settings.


#include "SFGA_Hero_Base.h"

#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"

USFGA_Hero_Base::USFGA_Hero_Base(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

void USFGA_Hero_Base::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const float AbilityLevel = GetAbilityLevel(Handle, ActorInfo);
	const float BaseCooldown = BaseCooldownDuration.GetValueAtLevel(AbilityLevel);

	// BaseCooldownDuration이 설정되지 않았으면 기본 ApplyCooldown 사용
	if (BaseCooldown <= 0.f)
	{
		Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
		return;
	}

	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGE->GetClass(), AbilityLevel);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(SFGameplayTags::Data_Cooldown_Base, BaseCooldown);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
}
