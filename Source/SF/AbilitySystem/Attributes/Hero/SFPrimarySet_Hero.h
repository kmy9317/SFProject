// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/SFPrimarySet.h"
#include "SFPrimarySet_Hero.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFPrimarySet_Hero : public USFPrimarySet
{
	GENERATED_BODY()
public:
	USFPrimarySet_Hero();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	
	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const override;
	
public:
	ATTRIBUTE_ACCESSORS(ThisClass, Mana);
	ATTRIBUTE_ACCESSORS(ThisClass, MaxMana);
	ATTRIBUTE_ACCESSORS(ThisClass, Stamina);
	ATTRIBUTE_ACCESSORS(ThisClass, MaxStamina);
	ATTRIBUTE_ACCESSORS(ThisClass, CooldownRate);
	ATTRIBUTE_ACCESSORS(ThisClass, ManaRegen);
	ATTRIBUTE_ACCESSORS(ThisClass, StaminaRegen);
	ATTRIBUTE_ACCESSORS(ThisClass, ManaReduction)

protected:
	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);
	
	UFUNCTION()
	void OnRep_CooldownRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ManaRegen(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_StaminaRegen(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ManaReduction(const FGameplayAttributeData& OldValue);

public:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Mana, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Mana;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxMana, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData MaxMana;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Stamina, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Stamina;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxStamina, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData MaxStamina;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_CooldownRate, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData CooldownRate;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_ManaRegen, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData ManaRegen;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_StaminaRegen, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData StaminaRegen;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_ManaReduction, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData ManaReduction;
};
