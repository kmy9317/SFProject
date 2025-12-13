// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/SFCharacterBase.h"
#include "SFHero.generated.h"

/**
 * 
 */
UCLASS()
class SF_API ASFHero : public ASFCharacterBase
{
	GENERATED_BODY()
public:
	ASFHero(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override {}
};
