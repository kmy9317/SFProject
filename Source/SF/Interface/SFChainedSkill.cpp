#include "SFChainedSkill.h"

#include "AbilitySystemComponent.h"

int32 ISFChainedSkill::GetCurrentChain() const
{
	const UAbilitySystemComponent* ASC = GetChainASC();
	TSubclassOf<UGameplayEffect> ComboStateClass = GetComboStateEffectClass();
        
	if (!ASC || !ComboStateClass)
	{
		return 0;
	}
	return ASC->GetGameplayEffectCount(ComboStateClass, nullptr);
}

bool ISFChainedSkill::CanContinueChain() const
{
	return GetCurrentChain() < GetChainConfigs().Num();
}

bool ISFChainedSkill::IsLastChain(int32 ChainIndex) const
{
	return ChainIndex >= (GetChainConfigs().Num() - 1);
}

FActiveGameplayEffectHandle ISFChainedSkill::ApplyComboState(UGameplayAbility* SourceAbility)
{
	UAbilitySystemComponent* ASC = GetChainASC();
	TSubclassOf<UGameplayEffect> ComboStateClass = GetComboStateEffectClass();
        
	if (!ASC || !SourceAbility || !ComboStateClass)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectSpecHandle SpecHandle = SourceAbility->MakeOutgoingGameplayEffectSpec(ComboStateClass);
	if (SpecHandle.IsValid())
	{
		return ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
	return FActiveGameplayEffectHandle();
}

void ISFChainedSkill::RemoveComboState()
{
	UAbilitySystemComponent* ASC = GetChainASC();
	TSubclassOf<UGameplayEffect> ComboStateClass = GetComboStateEffectClass();
        
	if (!ASC || !ComboStateClass)
	{
		return;
	}

	FGameplayEffectQuery Query;
	Query.EffectDefinition = ComboStateClass;
	ASC->RemoveActiveEffects(Query);
}

bool ISFChainedSkill::ApplyChainCost(int32 ChainIndex, UGameplayAbility* SourceAbility)
{
	const TArray<FSFChainConfig>& Configs = GetChainConfigs();
	if (!Configs.IsValidIndex(ChainIndex))
	{
		return false;
	}

	const FSFChainConfig& ChainConfig = Configs[ChainIndex];
	if (!ChainConfig.CostEffect)
	{
		return true;
	}

	UAbilitySystemComponent* ASC = GetChainASC();
	if (!ASC || !SourceAbility)
	{
		return false;
	}

	FGameplayEffectSpecHandle CostSpec = SourceAbility->MakeOutgoingGameplayEffectSpec(ChainConfig.CostEffect);
	if (CostSpec.IsValid())
	{
		FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*CostSpec.Data.Get());
		return Handle.IsValid();
	}
	return false;
}

void ISFChainedSkill::ApplyChainEffects(int32 ChainIndex, UGameplayAbility* SourceAbility)
{
	const TArray<FSFChainConfig>& Configs = GetChainConfigs();
	if (!Configs.IsValidIndex(ChainIndex))
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetChainASC();
	if (!ASC || !SourceAbility)
	{
		return;
	}

	TArray<FActiveGameplayEffectHandle>& Handles = GetAppliedChainEffectHandles();

	for (const TSubclassOf<UGameplayEffect>& EffectClass : Configs[ChainIndex].ChainEffects)
	{
		if (EffectClass)
		{
			FGameplayEffectSpecHandle SpecHandle = SourceAbility->MakeOutgoingGameplayEffectSpec(EffectClass);
			if (SpecHandle.IsValid())
			{
				FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				if (Handle.IsValid())
				{
					Handles.Add(Handle);
				}
			}
		}
	}
}

void ISFChainedSkill::RemoveChainEffects()
{
	UAbilitySystemComponent* ASC = GetChainASC();
	if (!ASC)
	{
		return;
	}

	TArray<FActiveGameplayEffectHandle>& Handles = GetAppliedChainEffectHandles();
	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}
	Handles.Empty();
}

void ISFChainedSkill::CompleteCombo(UGameplayAbility* SourceAbility)
{
	RemoveComboState();

	UAbilitySystemComponent* ASC = GetChainASC();
	TSubclassOf<UGameplayEffect> CooldownClass = GetCooldownEffectClass();
        
	if (ASC && SourceAbility && CooldownClass)
	{
		FGameplayEffectSpecHandle CooldownSpec = SourceAbility->MakeOutgoingGameplayEffectSpec(CooldownClass);
		if (CooldownSpec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*CooldownSpec.Data.Get());
		}
	}
}


