// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "SFAttributeSet.h"
#include "SFPrimarySet.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFPrimarySet : public USFAttributeSet
{
	GENERATED_BODY()

public:
	USFPrimarySet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const override;

public:
	ATTRIBUTE_ACCESSORS(ThisClass, Health);
	ATTRIBUTE_ACCESSORS(ThisClass, MaxHealth);
	ATTRIBUTE_ACCESSORS(ThisClass, MoveSpeed);
	ATTRIBUTE_ACCESSORS(ThisClass, Damage);

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);

private:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Health;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData MaxHealth;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MoveSpeed, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData MoveSpeed;

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Damage;
};
