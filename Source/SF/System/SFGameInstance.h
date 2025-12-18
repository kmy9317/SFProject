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
 * 개선: 포인터 대신 값 복사 방식으로 변경하여 안전성 향상
 */
USTRUCT()
struct FAbilityDataWrapper
{
	GENERATED_BODY()

	// 타입 정보
	EAbilityType Type = EAbilityType::None;

	// 실제 데이터 (값 복사)
	// Attack 타입일 경우 FEnemyAttackAbilityData로 캐스팅 가능
	FEnemyAttackAbilityData AttackData;

	// 추후 다른 타입 추가 시 여기에 추가
	// FEnemyDefensiveAbilityData DefensiveData;
	// FEnemyBuffAbilityData BuffData;

	FAbilityDataWrapper() = default;

	// Attack 타입 생성자
	explicit FAbilityDataWrapper(const FEnemyAttackAbilityData& InAttackData)
		: Type(EAbilityType::Attack)
		, AttackData(InAttackData)
	{}

	// 타입별 안전한 데이터 가져오기
	const FAbilityBaseData* GetBaseData() const
	{
		switch (Type)
		{
		case EAbilityType::Attack:
			return &AttackData;
		default:
			return nullptr;
		}
	}

	// Attack 데이터 가져오기 (타입 체크 포함)
	const FEnemyAttackAbilityData* GetAttackData() const
	{
		return (Type == EAbilityType::Attack) ? &AttackData : nullptr;
	}

	bool IsValid() const { return Type != EAbilityType::None; }
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
	void InitTeamAttitudeSolver();

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
