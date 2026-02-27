#include "SFGA_Hero_Parrying_A.h"

#include "AbilitySystemComponent.h"

void USFGA_Hero_Parrying_A::OnChainMontageCompleted()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC)
    {
        Super::OnChainMontageCompleted();
        return;
    }

    const bool bParrySuccess = ASC->HasMatchingGameplayTag(ParryEventTag);

    // 패링 실패 → 부모 로직
    if (!bParrySuccess)
    {
        Super::OnChainMontageCompleted();
        return;
    }

    // 추가 GE 적용 (1회만)
    if (HasAuthority(&CurrentActivationInfo)
        && ParrySuccessExtraEffect
        && ParryExtraEffectAppliedTag.IsValid()
        && !ASC->HasMatchingGameplayTag(ParryExtraEffectAppliedTag))
    {
        FGameplayEffectSpecHandle ExtraSpecHandle =
            MakeOutgoingGameplayEffectSpec(ParrySuccessExtraEffect, GetAbilityLevel());

        if (ExtraSpecHandle.IsValid())
        {
            ASC->ApplyGameplayEffectSpecToSelf(*ExtraSpecHandle.Data.Get());
            ASC->AddLooseGameplayTag(ParryExtraEffectAppliedTag);
            UE_LOG(LogTemp, Warning, TEXT("[Parry_A] Extra GE Applied ONCE"));
        }
    }

    // 패링 시도 완료 (Index 0) → 쿨다운 없이 카운터 대기
    if (!IsLastChain(ExecutingChainIndex))
    {
        Super::OnChainMontageCompleted();
        return;
    }

    // 카운터 완료 (Index 1, 마지막) → 90% 감소 쿨타임 적용
    TSubclassOf<UGameplayEffect> CooldownClass = GetCompleteCooldownEffectClass();
    if (!CooldownClass)
    {
        CooldownClass = GetTimeoutCooldownEffectClass();
    }

    if (CooldownClass)
    {
        float BaseDuration = GetCompleteCooldownDuration();
        if (BaseDuration <= 0.f)
        {
            BaseDuration = GetTimeoutCooldownDuration();
        }
        
        const float Scale = FMath::Clamp(ParrySuccessCooldownScale, 0.0f, 1.0f);
        const float FinalDuration = BaseDuration * Scale;

        ApplyChainCooldownInternal(this, CooldownClass, FinalDuration);
        
        UE_LOG(LogTemp, Warning, TEXT("[Parry_A] Counter Complete → %.0f%% Reduced Cooldown"), 
            (1.f - Scale) * 100.f);
    }

    RemoveChainEffects();
    RestoreSlidingMode();
    RemoveComboState(this);

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Hero_Parrying_A::OnComboStateRemoved(const FActiveGameplayEffect& RemovedEffect)
{
    // ComboState GE인지 확인
    TSubclassOf<UGameplayEffect> ComboStateClass = GetComboStateEffectClass();
    if (!ComboStateClass || RemovedEffect.Spec.Def->GetClass() != ComboStateClass)
    {
        return;
    }

    UAbilitySystemComponent* ASC = GetChainASC();
    if (!ASC)
    {
        return;
    }

    // 쿨다운 이미 적용됨 → 스킵 (CompleteCombo에서 처리됨)
    FGameplayTagContainer CDTags = GetChainedSkillCooldownTags();
    if (CDTags.Num() > 0 && ASC->HasAnyMatchingGameplayTags(CDTags))
    {
        return;
    }

    // 패링 성공 여부 확인
    const bool bParrySuccess = ASC->HasMatchingGameplayTag(ParryEventTag);

    TSubclassOf<UGameplayEffect> CooldownClass = GetTimeoutCooldownEffectClass();
    if (!CooldownClass)
    {
        // 쿨다운 클래스 없어도 델리게이트 해제
        UnbindComboStateRemovedDelegate();
        return;
    }

    float BaseDuration = GetTimeoutCooldownDuration();
    
    // 패링 성공 시 90% 감소
    if (bParrySuccess)
    {
        const float Scale = FMath::Clamp(ParrySuccessCooldownScale, 0.0f, 1.0f);
        BaseDuration *= Scale;
        UE_LOG(LogTemp, Warning, TEXT("[Parry_A] Timeout → %.0f%% Reduced Cooldown"), (1.f - Scale) * 100.f);
    }

    ApplyChainCooldownInternal(this, CooldownClass, BaseDuration);

    // 타임아웃 쿨다운 적용 후 델리게이트 해제
    UnbindComboStateRemovedDelegate();
}

void USFGA_Hero_Parrying_A::EndAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,bool bReplicateEndAbility,bool bWasCancelled)
{
	//태그 제거는 EndAbility에서 처리
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && ParryExtraEffectAppliedTag.IsValid())
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		ASC->RemoveLooseGameplayTag(ParryExtraEffectAppliedTag);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
