#include "SFCooldownCalculation.h"

#include "GameplayEffectExecutionCalculation.h"
#include "AbilitySystem/Attributes/Hero/SFPrimarySet_Hero.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"

struct SFCooldownStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(CooldownRate);

	SFCooldownStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(USFPrimarySet_Hero, CooldownRate, Source, false);
	}
};

static const SFCooldownStatics& GetCooldownStatics()
{
	static SFCooldownStatics CooldownStatics;
	return CooldownStatics;
}

USFCooldownCalculation::USFCooldownCalculation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	const SFCooldownStatics& Statics = GetCooldownStatics();
	RelevantAttributesToCapture.Add(Statics.CooldownRateDef);
}

float USFCooldownCalculation::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const SFCooldownStatics& Statics = GetCooldownStatics();
	
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;

	// SetByCaller로 전달된 기본 쿨타임
	const float BaseCooldown = Spec.GetSetByCallerMagnitude(SFGameplayTags::Data_Cooldown_Base, false,10.0f);

	// CooldownRate 캡처 (0.0 ~ 0.8 범위, 값이 높을수록 감소량 증가)
	float CooldownRate = 0.f;
	GetCapturedAttributeMagnitude(Statics.CooldownRateDef, Spec, EvaluateParameters, CooldownRate);
	CooldownRate = FMath::Clamp(CooldownRate, 0.f, 0.8f);

	const float FinalCooldown = BaseCooldown * (1.f - CooldownRate);
	return FinalCooldown;
}
