// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SFOSSGameInstance.h"
#include "AbilitySystem/Abilities/Enemy/Data/EnemyAttributeData.h"
#include "AbilitySystem/Abilities/Enemy/Data/FEnemyAbilityBaseData.h"
#include "Engine/GameInstance.h"
#include "SFGameInstance.generated.h"


/**
 * Ability Data를 담는 Wrapper 구조체
 */
USTRUCT()
struct FAbilityDataWrapper
{
	GENERATED_BODY()

	
	FAbilityBaseData* Data = nullptr;
    
	// 타입 정보
	EAbilityType Type = EAbilityType::None;

	FAbilityDataWrapper() = default;
    
	FAbilityDataWrapper(FAbilityBaseData* InData, EAbilityType InType)
		: Data(InData), Type(InType)
	{}

	// 타입별 안전한 캐스팅 헬퍼 함수들
	template<typename T>
	T* GetAs()
	{
		return static_cast<T*>(Data);
	}

	template<typename T>
	const T* GetAs() const
	{
		return static_cast<const T*>(Data);
	}

	bool IsValid() const { return Data != nullptr; }
};
/**
 * 
 */
UCLASS()
class SF_API USFGameInstance : public USFOSSGameInstance
{
	GENERATED_BODY()

public:
	void StartMatch();
	void LoadLevelAndListen(TSoftObjectPtr<UWorld> Level);

	// Ability Data 검색 함수
	const FAbilityBaseData* FindAbilityData(FName AbilityID) const;
	FAbilityDataWrapper* FindAbilityDataWrapper(FName AbilityID);
	
protected:
	virtual void Init() override;

private:
	
	/*
	* TODO : 추후 다른 방식으로 Map 참조 및 로드 할 수 있음
	*/
	UPROPERTY(EditDefaultsOnly, Category = "SF|Map")
	TSoftObjectPtr<UWorld> MainMenuLevel;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Map")
	TSoftObjectPtr<UWorld> LobbyLevel;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Map")
	TSoftObjectPtr<UWorld> GameLevel;


public:
	TMap<FName,	FEnemyAttributeData> EnemyDataMap;

protected:
	// Enemy DataTable
	UPROPERTY(EditDefaultsOnly, Category = "SF|Data")
	UDataTable* EnemyDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Data")
	UDataTable* EnemyAbilityDataTable;

	TMap<FName, FAbilityDataWrapper> EnemyAbilityMap;

	


protected:
	void LoadEnemyDataTable();
};
