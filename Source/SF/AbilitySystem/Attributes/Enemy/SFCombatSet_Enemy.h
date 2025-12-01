// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/SFCombatSet.h"
#include "SFCombatSet_Enemy.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFCombatSet_Enemy : public USFCombatSet
{
	GENERATED_BODY()

public:
	USFCombatSet_Enemy();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	ATTRIBUTE_ACCESSORS(ThisClass, SightRadius);
	ATTRIBUTE_ACCESSORS(ThisClass, LoseSightRadius);
	ATTRIBUTE_ACCESSORS(ThisClass, MeleeRange);
	ATTRIBUTE_ACCESSORS(ThisClass, GuardRange);
	
protected:
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;


protected:
	// 속성 변경 시 네트워크 복제를 위한 함수들
	UFUNCTION()
	void OnRep_SightRadius(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_LoseSightRadius(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MeleeRange(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_GuardRange(const FGameplayAttributeData& OldValue);

private:
	// 시야 감지 범위
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SightRadius, meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData SightRadius;

	// 시야 상실 범위 (추적 중단 거리)
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_LoseSightRadius, meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData LoseSightRadius;

	// 근접 공격 가능 거리 (Attack)
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MeleeRange, meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData MeleeRange;

	// 경계/원거리 공격 가능 거리 (Guard)
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_GuardRange, meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData GuardRange;
};
