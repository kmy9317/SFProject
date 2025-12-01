// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/FEnemyAbilityBaseData.h"
#include "UObject/Object.h"
#include "SFEnemyAbilityInitializer.generated.h"


struct  FGameplayAbilitySpec;
/**
 * Enemy Ability 초기화를 담당하는 유틸리티 클래스
 */
UCLASS()
class SF_API USFEnemyAbilityInitializer : public UObject
{
	GENERATED_BODY()

public:
	/** AbilitySpec에 Data 적용 */
	static void ApplyAbilityData(FGameplayAbilitySpec& Spec, const FAbilityBaseData& Data);

private:
	/** 맵 초기화 */
	static void InitializeMap(TMap<EAbilityType, TFunction<void(FGameplayAbilitySpec&, const FAbilityBaseData&)>>& Map);
    
	/** 공격형 Ability 초기화 */
	static void InitializeAttackAbility(FGameplayAbilitySpec& Spec, const FAbilityBaseData& Data);
    

};