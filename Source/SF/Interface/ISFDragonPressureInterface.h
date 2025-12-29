// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "ISFDragonPressureInterface.generated.h"


 //Dragon Pressure 타입 정의
 // 각 공격이 플레이어에게 부여하는 방향성 압박

UENUM(BlueprintType)
enum class EDragonPressureType : uint8
{
	None        UMETA(DisplayName = "None"),
	Forward     UMETA(DisplayName = "Forward Pressure"),   // 전방 압박 (앞으로 가면 위험)
	Back        UMETA(DisplayName = "Back Pressure"),      // 후방 압박 (뒤로 가면 위험)
	All         UMETA(DisplayName = "All Direction Pressure") // 전방향 압박 (주변 전체 위험)
};


UINTERFACE(MinimalAPI, Blueprintable)
class USFDragonPressureInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Dragon Pressure 인터페이스
 * Dragon 스킬이 플레이어에게 방향성 압박(Pressure)을 적용하기 위한 인터페이스
 */
class SF_API ISFDragonPressureInterface
{
	GENERATED_BODY()

public:
	virtual EDragonPressureType GetPressureType() const = 0;

	virtual float GetPressureDuration() const = 0;
	
	virtual TSubclassOf<UGameplayEffect> GetPressureEffectClass() const = 0;
	
	virtual void ApplyPressureToTarget(AActor* Target);

	static FGameplayTag PressureTypeToTag(EDragonPressureType Type);
	
	static bool HasPressure(AActor* Target, EDragonPressureType Type);
};

