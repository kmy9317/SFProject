// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "SFAttributeSet.h"
#include "SFCombatSet.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFCombatSet : public USFAttributeSet
{
	GENERATED_BODY()

public:
	USFCombatSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const override;

public:
	ATTRIBUTE_ACCESSORS(ThisClass, AttackPower);
	ATTRIBUTE_ACCESSORS(ThisClass, Defense);
	ATTRIBUTE_ACCESSORS(ThisClass, AttackSpeed);
	ATTRIBUTE_ACCESSORS(ThisClass, CriticalDamage);
	ATTRIBUTE_ACCESSORS(ThisClass, CriticalChance);
	ATTRIBUTE_ACCESSORS(USFCombatSet, IncomingDamageMultiplier)

protected:
	UFUNCTION()
	void OnRep_AttackPower(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Defense(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_AttackSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CriticalDamage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CriticalChance(const FGameplayAttributeData& OldValue);

public:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_AttackPower, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData AttackPower;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Defense, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Defense;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_AttackSpeed, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData AttackSpeed;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_CriticalDamage, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData CriticalDamage;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_CriticalChance, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData CriticalChance;

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData IncomingDamageMultiplier;
};
