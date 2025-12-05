// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "SFGameplayEffectContext.generated.h"

USTRUCT(BlueprintType)
struct FSFGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

public:

	// ----- Setters / Getters -----
	bool IsCriticalHit() const {return bIsCriticalHit ; }
	bool IsBlockHit() const { return bIsBlockedHit ; }

	void SetIsCriticalHit(bool bInIsCriticalHit) {bIsCriticalHit = bInIsCriticalHit ; }
	void SetIsBlockHit(bool bInIsBlockHit) {bIsBlockedHit = bInIsBlockHit ; }

	// ----- FGameplayEffectContext Override -----
	virtual UScriptStruct* GetScriptStruct() const override;


	virtual FGameplayEffectContext* Duplicate() const override;
	
	
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;
	
protected:

	UPROPERTY()
	bool bIsBlockedHit = false;
	
	UPROPERTY()
	bool bIsCriticalHit = false;
	
};

template<>
struct TStructOpsTypeTraits<FSFGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FSFGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
