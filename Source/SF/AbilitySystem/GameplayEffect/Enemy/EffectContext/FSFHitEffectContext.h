// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "FSFHitEffectContext.generated.h"

USTRUCT()
struct FSFHitEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FVector AttackDirection = FVector::ZeroVector;

	UPROPERTY()
	FVector HitLocation = FVector::ZeroVector;

	// ----- Setters / Getters -----
	void SetAttackDirection(const FVector& InDirection) { AttackDirection = InDirection; }
	void SetAttackLocation(const FVector& InLocation) { HitLocation = InLocation; }

	const FVector& GetAttackDirection() const { return AttackDirection; }
	const FVector& GetHitLocation() const { return HitLocation; }

	// ----- FGameplayEffectContext Override -----
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FSFHitEffectContext::StaticStruct();
	}

	virtual FGameplayEffectContext* Duplicate() const override
	{
		FSFHitEffectContext* NewContext = new FSFHitEffectContext(*this);
		return NewContext;
	}

	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override
	{
		bool ParentSuccess = FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);

		Ar << AttackDirection;
		Ar << HitLocation;

		bOutSuccess = ParentSuccess;
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FSFHitEffectContext> : public TStructOpsTypeTraitsBase2<FSFHitEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
