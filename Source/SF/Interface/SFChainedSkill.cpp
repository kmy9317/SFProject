#include "SFChainedSkill.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
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
		// 클라이언트의 즉각적인 UI 반응을 위해 존재
		BroadcastChainStateChanged(SourceAbility, 0);
	}
}

bool ISFChainedSkill::HandleComboStateRemoved(UGameplayAbility* SourceAbility, const FActiveGameplayEffect& RemovedEffect)
{
	TSubclassOf<UGameplayEffect> ComboStateClass = GetComboStateEffectClass();
	if (!ComboStateClass)
	{
		return false;
	}
    
	// ComboState GE인지 확인
	if (RemovedEffect.Spec.Def->GetClass() != ComboStateClass)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = GetChainASC();
	if (!ASC)
	{
		return false;
	}

	// 쿨다운이 이미 적용되어 있으면 스킵 (CompleteCombo에서 이미 처리됨)
	FGameplayTagContainer CDTags = GetChainedSkillCooldownTags();
	if (CDTags.Num() > 0 && ASC->HasAnyMatchingGameplayTags(CDTags))
	{
		return false;
	}

	// 타임아웃 쿨다운 적용
	ApplyTimeoutCooldown(SourceAbility);
	return true;
}

bool ISFChainedSkill::CheckChainCost(int32 ChainIndex, UAbilitySystemComponent* ASC, float AbilityLevel, const FGameplayEffectContextHandle& ContextHandle, FGameplayTagContainer* OptionalRelevantTags) const
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

	if (!ASC)
	{
		return false;
	}

	UGameplayEffect* CostGE = ChainConfig.CostEffect.GetDefaultObject();
	if (!CostGE)
	{
		return true;
	}

	if (!ASC->CanApplyAttributeModifiers(CostGE, AbilityLevel, ContextHandle))
	{
		const FGameplayTag& CostTag = UAbilitySystemGlobals::Get().ActivateFailCostTag;
		if (OptionalRelevantTags && CostTag.IsValid())
		{
			OptionalRelevantTags->AddTag(CostTag);
		}
		return false;
	}

	return true;
}

UGameplayEffect* ISFChainedSkill::GetChainCostEffect(int32 ChainIndex) const
{
	const TArray<FSFChainConfig>& Configs = GetChainConfigs();
	if (!Configs.IsValidIndex(ChainIndex))
	{
		return nullptr;
	}

	const FSFChainConfig& ChainConfig = Configs[ChainIndex];
	if (!ChainConfig.CostEffect)
	{
		return nullptr;
	}

	return ChainConfig.CostEffect.GetDefaultObject();
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
	// Complete 쿨다운 적용 (SetByCaller 방식)
	ApplyChainCooldownInternal(SourceAbility, GetCompleteCooldownEffectClass(), GetCompleteCooldownDuration());

	// OnComboStateRemoved 콜백에서 쿨다운 체크 시 이미 적용되어 있으므로 타임아웃 쿨타임 충돌x
	RemoveComboState(SourceAbility);
}

void ISFChainedSkill::ApplyTimeoutCooldown(UGameplayAbility* SourceAbility)
{
	ApplyChainCooldownInternal(SourceAbility, GetTimeoutCooldownEffectClass(), GetTimeoutCooldownDuration());
}

void ISFChainedSkill::ApplyChainCooldownInternal(UGameplayAbility* SourceAbility, TSubclassOf<UGameplayEffect> CooldownGEClass, float Duration)
{
	UAbilitySystemComponent* ASC = GetChainASC();
	if (!ASC || !SourceAbility || !CooldownGEClass || Duration <= 0.f)
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(SourceAbility);
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(CooldownGEClass, GetChainAbilityLevel(), ContextHandle);

	if (SpecHandle.IsValid())
	{
		// SetByCaller로 기본 쿨타임 전달 → USFCooldownCalculation에서 CooldownRate 적용
		SpecHandle.Data->SetSetByCallerMagnitude(SFGameplayTags::Data_Cooldown_Base, Duration);
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
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

	if (UGameplayMessageSubsystem::HasInstance(SourceAbility))
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(World);
		MessageSubsystem.BroadcastMessage(SFGameplayTags::Message_Skill_ChainStateChanged, Message);
	}
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


