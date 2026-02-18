#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "SFTargetDataTypes.generated.h"

USTRUCT(BlueprintType)
struct SF_API FSFGameplayAbilityTargetData_ChargePhase : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

public:
	FSFGameplayAbilityTargetData_ChargePhase()
		: PhaseIndex(0)
		, RushTargetLocation(FVector::ZeroVector)
		, RushTargetRotation(FRotator::ZeroRotator)
	{
	}

	FSFGameplayAbilityTargetData_ChargePhase(const int32 InPhaseIndex, const FVector& InRushTargetLocation, const FRotator& InRushTargetRotation)
		: PhaseIndex(InPhaseIndex)
		, RushTargetLocation(InRushTargetLocation)
		, RushTargetRotation(InRushTargetRotation)
	{
	}

	UPROPERTY()
	int32 PhaseIndex;

	UPROPERTY()
	FVector RushTargetLocation;

	UPROPERTY()
	FRotator RushTargetRotation;

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FSFGameplayAbilityTargetData_ChargePhase::StaticStruct();
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << PhaseIndex;
		RushTargetLocation.NetSerialize(Ar, Map, bOutSuccess);
		RushTargetRotation.NetSerialize(Ar, Map, bOutSuccess);
		bOutSuccess = true;
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FSFGameplayAbilityTargetData_ChargePhase> : public TStructOpsTypeTraitsBase2<FSFGameplayAbilityTargetData_ChargePhase>
{
	enum
	{
		WithNetSerializer = true
	};
};

USTRUCT(BlueprintType)
struct SF_API FSFGameplayAbilityTargetData_WarpDirection : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

public:
	FSFGameplayAbilityTargetData_WarpDirection()
		: WindupIndex(0)
		, WarpLocation(FVector::ZeroVector)
		, WarpRotation(FRotator::ZeroRotator)
	{
	}

	FSFGameplayAbilityTargetData_WarpDirection(int32 InWindupIndex, const FVector& InWarpLocation, const FRotator& InWarpRotation)
		: WindupIndex(InWindupIndex)
		, WarpLocation(InWarpLocation)
		, WarpRotation(InWarpRotation)
	{
	}

	// 몇 번째 Windup 구간인지
	UPROPERTY()
	int32 WindupIndex;

	// Warp 목표 위치
	UPROPERTY()
	FVector WarpLocation;

	// Warp 목표 회전 (방향)
	UPROPERTY()
	FRotator WarpRotation;

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FSFGameplayAbilityTargetData_WarpDirection::StaticStruct();
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << WindupIndex;
		WarpLocation.NetSerialize(Ar, Map, bOutSuccess);
		WarpRotation.NetSerialize(Ar, Map, bOutSuccess);
		bOutSuccess = true;
		return true;
	}
};
template<>
struct TStructOpsTypeTraits<FSFGameplayAbilityTargetData_WarpDirection> : public TStructOpsTypeTraitsBase2<FSFGameplayAbilityTargetData_WarpDirection>
{
	enum
	{
		WithNetSerializer = true
	};
};

USTRUCT(BlueprintType)
struct SF_API FSFGameplayAbilityTargetData_Location : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

public:
	FSFGameplayAbilityTargetData_Location()
	   : TargetLocation(FVector::ZeroVector)
	{
	}

	FSFGameplayAbilityTargetData_Location(const FVector& InTargetLocation)
	   : TargetLocation(InTargetLocation)
	{
	}

	UPROPERTY()
	FVector TargetLocation;

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FSFGameplayAbilityTargetData_Location::StaticStruct();
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		TargetLocation.NetSerialize(Ar, Map, bOutSuccess);
		bOutSuccess = true;
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FSFGameplayAbilityTargetData_Location>
	: public TStructOpsTypeTraitsBase2<FSFGameplayAbilityTargetData_Location>
{
	enum
	{
		WithNetSerializer = true
	 };
};