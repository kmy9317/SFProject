// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SFNetStatics.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFNetStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static uint8 GetPlayerCountPerTeam();
};
