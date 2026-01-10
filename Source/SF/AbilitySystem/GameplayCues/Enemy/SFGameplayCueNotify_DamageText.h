// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "SFGameplayCueNotify_DamageText.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGameplayCueNotify_DamageText : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	USFGameplayCueNotify_DamageText();

	
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	
};
