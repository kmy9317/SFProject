// Fill out your copyright notice in the Description page of Project Settings.

#include "ISFDragonPressureInterface.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/Enemy/Component/Boss_Dragon/SFDragonGameplayTags.h"

void ISFDragonPressureInterface::ApplyPressureToTarget(AActor* Target)
{
	if (!Target)
	{
		return;
	}
	
	TSubclassOf<UGameplayEffect> EffectClass = GetPressureEffectClass();
	if (!EffectClass)
	{
		return;
	}
	
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	
	if (!TargetASC)
	{
		return;
	}
	
	FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(
		EffectClass,
		1.0f,  
		ContextHandle
	);

	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[Pressure] Failed to create GameplayEffect spec"));
		return;
	}
	
	float Duration = GetPressureDuration();
	SpecHandle.Data->SetDuration(Duration, true);
	
	FActiveGameplayEffectHandle ActiveHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	
}

FGameplayTag ISFDragonPressureInterface::PressureTypeToTag(EDragonPressureType Type)
{
	switch (Type)
	{
		case EDragonPressureType::Forward:
			return SFGameplayTags::Dragon_Pressure_Forward;
			
		case EDragonPressureType::Back:
			return SFGameplayTags::Dragon_Pressure_Back;
			
		case EDragonPressureType::All:
			return SFGameplayTags::Dragon_Pressure_All;
			
		default:
			return FGameplayTag();
	}
}

bool ISFDragonPressureInterface::HasPressure(AActor* Target, EDragonPressureType Type)
{
	if (!Target)
		return false;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	
	if (!TargetASC)
		return false;

	FGameplayTag PressureTag = PressureTypeToTag(Type);
	return TargetASC->HasMatchingGameplayTag(PressureTag);
}

