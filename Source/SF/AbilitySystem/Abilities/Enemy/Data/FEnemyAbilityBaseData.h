// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "FEnemyAbilityBaseData.generated.h"





UENUM(BlueprintType)
enum class EAbilityType : uint8
{
	None,
	Attack,
	Defensive,
	Buff,
	Utility
};

USTRUCT(BlueprintType)
struct FAbilityBaseData : public FTableRowBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAbilityType AbilityType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class USFGameplayAbility> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Cooldown;
	


	FAbilityBaseData()
		: AbilityType(EAbilityType::None)
		, AbilityClass(nullptr)
		, Cooldown(1.f)
	{}
};

USTRUCT(BlueprintType)
struct FEnemyAttackAbilityData : public FAbilityBaseData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack")
	float BaseDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack")
	float AttackRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack")
	float MinAttackRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack")
	float AttackAngle;

	FEnemyAttackAbilityData()
		: BaseDamage(10.f)
		, AttackRange(200.f)
		, MinAttackRange(0.f)
		, AttackAngle(90.f)
	{}
};

