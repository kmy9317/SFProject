// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/SFPrimarySet.h"
#include "SFPrimarySet_Enemy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTakeDamageSignature, float, Damage, AActor*, DamageCauser);
/**
 * 
 */
UCLASS()
class SF_API USFPrimarySet_Enemy : public USFPrimarySet
{
	GENERATED_BODY()

public:
	USFPrimarySet_Enemy();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const override;
	
public:
	ATTRIBUTE_ACCESSORS(ThisClass, Stagger);
	ATTRIBUTE_ACCESSORS(ThisClass, MaxStagger);

	FOnTakeDamageSignature OnTakeDamageDelegate;

protected:
	UFUNCTION()
	void OnRep_Stagger(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxStagger(const FGameplayAttributeData& OldValue);

public:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Stagger, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Stagger;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxStagger, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData MaxStagger;

	
};
