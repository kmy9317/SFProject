// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Character/SFPawnData.h"
#include "Engine/DataTable.h"
#include "EnemyAttributeData.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FEnemyAttributeData : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackPower = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoveSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Defense = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CriticalDamage = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CriticalChance = 0.05f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxStagger = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SightRadius = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LoseSightRadius = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GuardRange = 100.f;
};
