// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/EffectExecutionCalculation/SFDamageEffectExecCalculation.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/SFCombatSet.h"
#include "AbilitySystem/Attributes/SFPrimarySet.h"
#include "AbilitySystem/GameplayEffect/SFGameplayEffectContext.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "AI/SFAIGameplayTags.h"


struct SFDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Damage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalChance);

	SFDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(USFPrimarySet, Damage, Source, true)
		DEFINE_ATTRIBUTE_CAPTUREDEF(USFCombatSet, AttackPower, Source, true)
		DEFINE_ATTRIBUTE_CAPTUREDEF(USFCombatSet, Defense, Target, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(USFCombatSet, CriticalDamage, Source, true)
		DEFINE_ATTRIBUTE_CAPTUREDEF(USFCombatSet, CriticalChance, Source, true)
	}
};
// Static 함수 추가 필요
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
	
}

void USFDamageEffectExecCalculation::Execute_Implementation(
    const FGameplayEffectCustomExecutionParameters& ExecutionParams,
    FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    // ASC 가져오기
    UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
    if (!TargetASC) return;
    UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
    if (!SourceASC) return;

    AActor* SourceAvatarActor = SourceASC->GetAvatarActor();
    AActor* TargetAvatarActor = TargetASC->GetAvatarActor();
    if (!SourceAvatarActor || !TargetAvatarActor) return;
	
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	float BaseDamage = Spec.GetSetByCallerMagnitude(SFGameplayTags::Data_Damage_BaseDamage,false,  0.f );
  
    
    FAggregatorEvaluateParameters EvalParams;
    EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
    
    const SFDamageStatics& DamageStatics = GetDamageStatics();
    
    // 공격력
    float AttackPower = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
        DamageStatics.AttackPowerDef, EvalParams, AttackPower);
    
    float FinalDamage = AttackPower + BaseDamage;
    
    // 크리티컬 판정
    float CriticalChance = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
        DamageStatics.CriticalChanceDef, EvalParams, CriticalChance);
    
    bool bIsCritical = FMath::FRand() < CriticalChance;
    
    if (bIsCritical)
    {
        float CriticalDamage = 0.f;
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
            DamageStatics.CriticalDamageDef, EvalParams, CriticalDamage);
        
        FinalDamage *= CriticalDamage;
        
        // 크리티컬일 경우  태그 추가 이거 Ui 표시 위해 
    	FGameplayEffectContextHandle ContextHandle = Spec.GetEffectContext();
    	if (FSFGameplayEffectContext* SFContext = static_cast<FSFGameplayEffectContext*>(ContextHandle.Get()))
    	{
    		SFContext->SetIsCriticalHit(true);
    	}
    }
    
    // 방어력  이거 방어력 100이면 최종데미지 50%로 만든다 
    float Defense = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
        DamageStatics.DefenseDef, EvalParams, Defense);
    Defense = FMath::Max(Defense, 0.0f);
    
    float DefenseReduction = Defense / (Defense + 100.0f);
    FinalDamage *= (1.0f - DefenseReduction);
    
    // 최종 데미지 적용
    if (FinalDamage > 0.0f)
    {
        OutExecutionOutput.AddOutputModifier(
            FGameplayModifierEvaluatedData(
                USFPrimarySet::GetDamageAttribute(),
                EGameplayModOp::Additive,
                FinalDamage
            )
        );
    }
}