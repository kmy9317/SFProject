#include "SFChainedSkill.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Messages/SFMessageGameplayTags.h"
#include "Messages/SFSkillInfoMessages.h"

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

FActiveGameplayEffectHandle ISFChainedSkill::ApplyComboState(UGameplayAbility* SourceAbility, int32 NextChainIndex)
{
	UAbilitySystemComponent* ASC = GetChainASC();
	TSubclassOf<UGameplayEffect> ComboStateClass = GetComboStateEffectClass();
        
	if (!ASC || !SourceAbility || !ComboStateClass)
	{
		return FActiveGameplayEffectHandle();
	}

	FActiveGameplayEffectHandle Handle;
    
	FGameplayEffectSpecHandle SpecHandle = SourceAbility->MakeOutgoingGameplayEffectSpec(ComboStateClass);
	if (SpecHandle.IsValid())
	{
		Handle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
    
	// Handle 유효성과 상관없이 브로드캐스트 (클라이언트 UI 갱신용)
	BroadcastChainStateChanged(SourceAbility, NextChainIndex);
    
	return Handle;
}

void ISFChainedSkill::RemoveComboState(UGameplayAbility* SourceAbility)
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

	if (SourceAbility)
	{
		BroadcastChainStateChanged(SourceAbility, 0);
	}
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
	RemoveComboState(SourceAbility);

	UAbilitySystemComponent* ASC = GetChainASC();
	TSubclassOf<UGameplayEffect> CooldownClass = GetCompleteCooldownEffectClass();
        
	if (ASC && SourceAbility && CooldownClass)
	{
		FGameplayEffectSpecHandle CooldownSpec = SourceAbility->MakeOutgoingGameplayEffectSpec(CooldownClass);
		if (CooldownSpec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*CooldownSpec.Data.Get());
		}
	}
}

void ISFChainedSkill::BroadcastChainStateChanged(UGameplayAbility* SourceAbility, int32 ChainIndex)
{
	if (!SourceAbility)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetChainASC();
	if (!ASC)
	{
		return;
	}

	UWorld* World = ASC->GetWorld();
	if (!World)
	{
		return;
	}

	FSFChainStateChangedMessage Message;
	Message.AbilitySpecHandle = SourceAbility->GetCurrentAbilitySpecHandle();
	Message.ChainIndex = ChainIndex;

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(World);
	MessageSubsystem.BroadcastMessage(SFGameplayTags::Message_Skill_ChainStateChanged, Message);
}

UTexture2D* ISFChainedSkill::GetChainIcon(int32 ChainIndex) const
{
	const TArray<FSFChainConfig>& Configs = GetChainConfigs();
	if (Configs.IsValidIndex(ChainIndex))
	{
		return Configs[ChainIndex].Icon;
	}
	return nullptr;
}


