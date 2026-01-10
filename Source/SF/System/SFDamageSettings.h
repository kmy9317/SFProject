// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SFDamageSettings.generated.h"

class USFDamageWidget;
/**
 * 
 */
UCLASS(Config=Game, DefaultConfig, meta = (DisplayName = "SF Damage System Settings"))
class SF_API USFDamageSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	
	UPROPERTY(Config, EditAnywhere, Category = "UI")
	TSoftClassPtr<USFDamageWidget> DamageWidgetClass;
};
