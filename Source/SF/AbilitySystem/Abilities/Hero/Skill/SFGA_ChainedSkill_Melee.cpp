#include "SFGA_ChainedSkill_Melee.h"

#include "AbilitySystemComponent.h"
#include "SFHeroSkillTags.h"
#include "SFLogChannels.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Weapons/Actor/SFEquipmentBase.h"

USFGA_ChainedSkill_Melee::USFGA_ChainedSkill_Melee(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool USFGA_ChainedSkill_Melee::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (ChainConfigs.Num() == 0)
	{
		return false;
	}
	
	return CanContinueChain();
}

UAbilitySystemComponent* USFGA_ChainedSkill_Melee::GetChainASC() const
{
	return GetAbilitySystemComponentFromActorInfo();
}

UGameplayEffect* USFGA_ChainedSkill_Melee::GetCooldownGameplayEffect() const
{
	if (TimeoutCooldownEffectClass)
	{
		return TimeoutCooldownEffectClass->GetDefaultObject<UGameplayEffect>();
	}
	return nullptr;
}

void USFGA_ChainedSkill_Melee::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	// 체인 스킬은 CommitAbility 시점에 쿨다운 적용하지 않음
	// Timeout 또는 Complete 시점에 ISFChainedSkill::ApplyChainCooldownInternal()로 적용
}

float USFGA_ChainedSkill_Melee::GetTimeoutCooldownDuration() const
{
	return TimeoutCooldownDuration.GetValueAtLevel(GetAbilityLevel());
}

float USFGA_ChainedSkill_Melee::GetCompleteCooldownDuration() const
{
	return CompleteCooldownDuration.GetValueAtLevel(GetAbilityLevel());
}

bool USFGA_ChainedSkill_Melee::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	return CheckChainCost(GetCurrentChain(), ASC, GetAbilityLevel(Handle, ActorInfo), MakeEffectContext(Handle, ActorInfo), OptionalRelevantTags);
}

void USFGA_ChainedSkill_Melee::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CostGE = GetChainCostEffect(GetCurrentChain());
	if (CostGE)
	{
		ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, CostGE, GetAbilityLevel(Handle, ActorInfo));
	}
}

void USFGA_ChainedSkill_Melee::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!GetChainASC())
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 쿨타임 초기화 관련 로직
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// 이미 바인딩돼 있으면 중복 방지
	if (!CooldownGEAddedHandle.IsValid())
	{
		CooldownGEAddedHandle = ASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &ThisClass::OnCooldownGEAdded);
	}

	// ComboState 제거 감지
	BindComboStateRemovedDelegate();
	
	ExecutingChainIndex = GetCurrentChain();
	ExecuteChainStep(ExecutingChainIndex);

	if (bAutoApplyComboState)
	{
		ApplyComboState(this, ExecutingChainIndex + 1);
	}
}

void USFGA_ChainedSkill_Melee::OnTrace(FGameplayEventData Payload)
{
	Super::OnTrace(Payload);
}

void USFGA_ChainedSkill_Melee::ExecuteChainStep(int32 ChainIndex)
{
	if (!ChainConfigs.IsValidIndex(ChainIndex))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	
	const FSFChainConfig& ChainConfig = ChainConfigs[ChainIndex];
	CurrentDamageMultiplier = ChainConfig.DamageMultiplier;

	if (ChainConfig.ChainSlidingMode != ESFSlidingMode::None)
	{
		ApplySlidingMode(ChainConfig.ChainSlidingMode);
	}

	UAnimMontage* MontageToPlay = ChainConfig.Montage;
	if (!MontageToPlay)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	ApplyChainEffects(ChainIndex, this);

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		FName(*FString::Printf(TEXT("ChainStep_%d"), ChainIndex)),
		MontageToPlay,
		1.0f,
		NAME_None,
		true
	);

	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnChainMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnChainMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnChainMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnChainMontageInterrupted);
		MontageTask->ReadyForActivation();
	}

	// Trace
	UAbilityTask_WaitGameplayEvent* TraceTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		SFGameplayTags::GameplayEvent_Tracing,
		nullptr,
		false,
		true
	);
	if (TraceTask)
	{
		TraceTask->EventReceived.AddDynamic(this, &ThisClass::OnTrace);
		TraceTask->ReadyForActivation();
	}
}

void USFGA_ChainedSkill_Melee::OnChainMontageCompleted()
{
	RemoveChainEffects();
	RestoreSlidingMode();

	if (IsLastChain(ExecutingChainIndex))
	{
		CompleteCombo(this);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_ChainedSkill_Melee::OnChainMontageInterrupted()
{
	RemoveChainEffects();
	RestoreSlidingMode();

	if (IsLastChain(ExecutingChainIndex))
	{
		CompleteCombo(this);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USFGA_ChainedSkill_Melee::BindComboStateRemovedDelegate()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    
	if (!ASC || ComboStateRemovedHandle.IsValid() || !ComboStateEffectClass)
	{
		return;
	}
	ComboStateRemovedHandle = ASC->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &ThisClass::OnComboStateRemoved);
}

void USFGA_ChainedSkill_Melee::UnbindComboStateRemovedDelegate()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    
	if (ASC && ComboStateRemovedHandle.IsValid())
	{
		ASC->OnAnyGameplayEffectRemovedDelegate().Remove(ComboStateRemovedHandle);
		ComboStateRemovedHandle.Reset();
	}
}

void USFGA_ChainedSkill_Melee::OnComboStateRemoved(const FActiveGameplayEffect& RemovedEffect)
{
	// 인터페이스의 핵심 로직 호출
	if (HandleComboStateRemoved(this, RemovedEffect))
	{
		UnbindComboStateRemovedDelegate();
	}
}

void USFGA_ChainedSkill_Melee::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		// ComboState가 남아있으면 델리게이트 유지 (타임아웃 감지 필요)
		bool bHasComboState = ComboStateEffectClass && ASC->GetGameplayEffectCount(ComboStateEffectClass, ASC) > 0;
		if (!bHasComboState)
		{
			// ComboState 없음 = CompleteCombo 완료
			UnbindComboStateRemovedDelegate();
		}

		if (CooldownGEAddedHandle.IsValid())
		{
			ASC->OnActiveGameplayEffectAddedDelegateToSelf.Remove(CooldownGEAddedHandle);
			CooldownGEAddedHandle.Reset();
		}
	}

	RestoreSlidingMode();
	ExecutingChainIndex = 0;
	CurrentDamageMultiplier = 1.0f;
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGA_ChainedSkill_Melee::OnCooldownGEAdded(
	UAbilitySystemComponent* TargetASC,
	const FGameplayEffectSpec& Spec,
	FActiveGameplayEffectHandle ActiveHandle)
{
	const FGameplayTagContainer* CDTags = GetCooldownTags();
	if (!CDTags || CDTags->IsEmpty())
	{
		return;
	}

	FGameplayTagContainer EffectTags;
	Spec.GetAllAssetTags(EffectTags);
	Spec.GetAllGrantedTags(EffectTags);

	if (!EffectTags.HasAny(*CDTags))
	{
		return;
	}

	// 🔥 Timeout / Complete 공통 처리
	TryProcCooldownReset_FromASC(TargetASC);

	if (CooldownGEAddedHandle.IsValid())
	{
		TargetASC->OnActiveGameplayEffectAddedDelegateToSelf.Remove(CooldownGEAddedHandle);
		CooldownGEAddedHandle.Reset();
	}
}

void USFGA_ChainedSkill_Melee::TryProcCooldownReset_FromASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	AActor* OwnerActor = ASC->GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	// 패시브 없으면 패스
	if (!ASC->HasMatchingGameplayTag(SFGameplayTags::Ability_Skill_Passive_CooldownReset))
	{
		return;
	}
	
	if (FMath::FRand() > 0.25f)
	{
		return;
	}

	const FGameplayTagContainer* CDTags = GetCooldownTags();
	if (!CDTags || CDTags->IsEmpty())
	{
		return;
	}

	// 쿨타임 초기화
	ASC->RemoveActiveEffectsWithTags(*CDTags);
}

const FGameplayTagContainer* USFGA_ChainedSkill_Melee::GetCooldownTags() const
{
	// 인스펙터에서 설정한 이 태그(Ability.Cooldown.Hero.Skill.Identity)를 기준으로 제거할 거라서
	return CooldownTags.IsEmpty() ? Super::GetCooldownTags() : &CooldownTags;
}
