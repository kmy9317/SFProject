// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"

#include "SFEnemyManagerComponent.generated.h"

class ASFEnemy;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllEnemiesDefeatedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyCountChangedDelegate, int32, AliveCount, int32, TotalCount);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFEnemyManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	USFEnemyManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "SF|Enemy")
	void RegisterEnemy(ASFEnemy* Enemy);

	UFUNCTION(BlueprintCallable, Category = "SF|Enemy")
	void UnregisterEnemy(ASFEnemy* Enemy);

	UFUNCTION(BlueprintCallable, Category = "SF|Enemy")
	void NotifyAllEnemiesSpawned();

	UFUNCTION(BlueprintPure, Category = "SF|Enemy")
	int32 GetAliveEnemyCount() const { return AliveEnemyCount; }

	UFUNCTION(BlueprintPure, Category = "SF|Enemy")
	int32 GetTotalEnemyCount() const { return TotalEnemyCount; }

protected:
	void CheckAllEnemiesDefeated();

	UFUNCTION()
	void OnRep_AliveEnemyCount();

	UFUNCTION()
	void OnRep_TotalEnemyCount();

public:
	
	// 모든 적 처치 시 브로드캐스트 
	UPROPERTY(BlueprintAssignable, Category = "SF|Events")
	FOnAllEnemiesDefeatedDelegate OnAllEnemiesDefeated;

	// 적 수 변경 시 브로드캐스트 (추후 필요시 UI용) 
	UPROPERTY(BlueprintAssignable, Category = "SF|Events")
	FOnEnemyCountChangedDelegate OnEnemyCountChanged;

private:
	// 현재 등록된 적들 (서버만 관리)
	UPROPERTY()
	TSet<TObjectPtr<ASFEnemy>> RegisteredEnemies;

	// 살아있는 적 수 (추후 필요시 UI 표시용) 
	UPROPERTY(ReplicatedUsing = OnRep_AliveEnemyCount)
	int32 AliveEnemyCount;

	UPROPERTY(ReplicatedUsing = OnRep_TotalEnemyCount)
	int32 TotalEnemyCount;

	bool bAllEnemiesSpawned;
	
	// 중복 확인용
	bool bStageCleared;
};
