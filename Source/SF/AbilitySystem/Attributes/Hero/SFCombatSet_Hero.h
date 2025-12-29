// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/SFCombatSet.h"
#include "SFCombatSet_Hero.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFCombatSet_Hero : public USFCombatSet
{
	GENERATED_BODY()

public:
	USFCombatSet_Hero();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const override;

public:
	ATTRIBUTE_ACCESSORS(ThisClass, Luck);
	ATTRIBUTE_ACCESSORS(ThisClass, ReviveGauge)

protected:
	UFUNCTION()
	void OnRep_Luck(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ReviveGauge(const FGameplayAttributeData& OldValue);

private:
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Luck, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Luck;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_ReviveGauge, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData ReviveGauge;

};
