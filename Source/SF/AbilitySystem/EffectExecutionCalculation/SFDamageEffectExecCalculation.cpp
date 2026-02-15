// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/EffectExecutionCalculation/SFDamageEffectExecCalculation.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/SFCombatSet.h"
#include "AbilitySystem/Attributes/SFPrimarySet.h"
#include "AbilitySystem/GameplayEffect/SFGameplayEffectContext.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AI/SFAIGameplayTags.h"
#include "Character/SFCharacterGameplayTags.h"


struct SFDamageStatics
{
    DECLARE_ATTRIBUTE_CAPTUREDEF(Damage);
    DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
    DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
    DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamage);
    DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalChance);
    DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamageMultiplier);

    SFDamageStatics()
    {
        DEFINE_ATTRIBUTE_CAPTUREDEF(USFPrimarySet, Damage, Source, true)
        DEFINE_ATTRIBUTE_CAPTUREDEF(USFCombatSet,  AttackPower, Source, true)
        DEFINE_ATTRIBUTE_CAPTUREDEF(USFCombatSet, Defense, Target, false)
        DEFINE_ATTRIBUTE_CAPTUREDEF(USFCombatSet, CriticalDamage, Source, true)
        DEFINE_ATTRIBUTE_CAPTUREDEF(USFCombatSet, CriticalChance, Source, true)
        DEFINE_ATTRIBUTE_CAPTUREDEF(USFCombatSet, IncomingDamageMultiplier, Target, false);
    }
};

static const SFDamageStatics& GetDamageStatics()
{
    static SFDamageStatics DamageStatics;
    return DamageStatics;
}

USFDamageEffectExecCalculation::USFDamageEffectExecCalculation()
{
    const SFDamageStatics& DamageStatics = GetDamageStatics();
    RelevantAttributesToCapture.Add(DamageStatics.DamageDef);
    RelevantAttributesToCapture.Add(DamageStatics.AttackPowerDef);
    RelevantAttributesToCapture.Add(DamageStatics.DefenseDef);
    RelevantAttributesToCapture.Add(DamageStatics.CriticalDamageDef);
    RelevantAttributesToCapture.Add(DamageStatics.CriticalChanceDef);
    RelevantAttributesToCapture.Add(DamageStatics.IncomingDamageMultiplierDef);
}

void USFDamageEffectExecCalculation::Execute_Implementation(
    const FGameplayEffectCustomExecutionParameters& ExecutionParams,
    FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    
    UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
    UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
    if (!TargetASC || !SourceASC) return;

    AActor* SourceActor = SourceASC->GetAvatarActor();
    AActor* TargetActor = TargetASC->GetAvatarActor();
    if (!SourceActor || !TargetActor) return;

    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
    
    float FinalDamage = CalculateBaseDamage(ExecutionParams, Spec);
    
    FinalDamage = ApplyCritical(ExecutionParams, Spec, FinalDamage);
  
    FinalDamage = ApplyDefense(ExecutionParams, FinalDamage);

    FinalDamage = ApplyIncomingDamageMultiplier(ExecutionParams, FinalDamage);
    
    OutputFinalDamage(FinalDamage, OutExecutionOutput);
}


float USFDamageEffectExecCalculation::CalculateBaseDamage(
    const FGameplayEffectCustomExecutionParameters& ExecutionParams,
    const FGameplayEffectSpec& Spec) const
{
    // SetByCaller로 받은 기본 데미지
    float BaseDamage = Spec.GetSetByCallerMagnitude(
        SFGameplayTags::Data_Damage_BaseDamage, false, 0.f);

    // 공격력 가져오기
    FAggregatorEvaluateParameters EvalParams;
    EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    const SFDamageStatics& DamageStatics = GetDamageStatics();

    float AttackPower = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
        DamageStatics.AttackPowerDef, EvalParams, AttackPower);

    return BaseDamage + AttackPower;
}

float USFDamageEffectExecCalculation::ApplyCritical(
    const FGameplayEffectCustomExecutionParameters& ExecutionParams,
    const FGameplayEffectSpec& Spec,
    float InDamage) const
{
    const SFDamageStatics& DamageStatics = GetDamageStatics();

    FAggregatorEvaluateParameters EvalParams;
    EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    // 크리티컬 확률 가져오기
    float CriticalChance = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
        DamageStatics.CriticalChanceDef, EvalParams, CriticalChance);
    
    bool bIsCritical = FMath::FRand() < CriticalChance;

    if (bIsCritical)
    {
        float CriticalDamage = 0.f;
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
            DamageStatics.CriticalDamageDef, EvalParams, CriticalDamage);
        
        FGameplayEffectContextHandle ContextHandle = Spec.GetEffectContext();
        if (FSFGameplayEffectContext* SFContext = static_cast<FSFGameplayEffectContext*>(ContextHandle.Get()))
        {
            SFContext->SetIsCriticalHit(true);
        }

        return InDamage * CriticalDamage;
    }

    return InDamage;
}

float USFDamageEffectExecCalculation::ApplyDefense(
    const FGameplayEffectCustomExecutionParameters& ExecutionParams,
    float InDamage) const
{
    const SFDamageStatics& DamageStatics = GetDamageStatics();
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

    FAggregatorEvaluateParameters EvalParams;
    EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    // 방어력 가져오기
    float Defense = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
        DamageStatics.DefenseDef, EvalParams, Defense);

    Defense = FMath::Max(Defense, 0.0f);

    // 방어력 공식: Defense / (Defense + 100)
    float DefenseReduction = Defense / (Defense + 100.0f);

    return InDamage * (1.0f - DefenseReduction);
}

float USFDamageEffectExecCalculation::ApplyIncomingDamageMultiplier(const FGameplayEffectCustomExecutionParameters& ExecutionParams, float InDamage) const
{
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

    FAggregatorEvaluateParameters EvalParams;
    EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    float IncomingDamageMultiplier = 1.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
        GetDamageStatics().IncomingDamageMultiplierDef,
        EvalParams,
        IncomingDamageMultiplier);
    IncomingDamageMultiplier = FMath::Max(IncomingDamageMultiplier, 0.0f);

    return InDamage * IncomingDamageMultiplier;
}

void USFDamageEffectExecCalculation::OutputFinalDamage(float FinalDamage, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    if (FinalDamage > 0.0f)
    {
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData( USFPrimarySet::GetDamageAttribute(), EGameplayModOp::Additive, FinalDamage));
    }
}