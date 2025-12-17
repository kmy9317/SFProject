// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "SFGA_Hero_Base.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGA_Hero_Base : public USFGameplayAbility
{
	GENERATED_BODY()
public:
	USFGA_Hero_Base(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
