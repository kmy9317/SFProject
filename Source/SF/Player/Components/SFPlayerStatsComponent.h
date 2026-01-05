#pragma once

#include "CoreMinimal.h"
#include "Components/PlayerStateComponent.h"
#include "SFPlayerStatsComponent.generated.h"

/**
 * 게임 전체 누적 통계 
 */
USTRUCT(BlueprintType)
struct FSFPlayerStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float TotalDamageDealt = 0.f;

	UPROPERTY(BlueprintReadOnly)
	int32 TotalDownedCount = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 TotalReviveCount = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 TotalEnemiesKilled = 0;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFPlayerStatsComponent : public UPlayerStateComponent
{
	GENERATED_BODY()

public:
	USFPlayerStatsComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "SF|Stats")
	static USFPlayerStatsComponent* FindPlayerStatsComponent(const AActor* Actor);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SF|Stats")
	void AddDamageDealt(float Amount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SF|Stats")
	void IncrementDownedCount();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SF|Stats")
	void IncrementReviveCount();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SF|Stats")
	void IncrementKillCount();

	UFUNCTION(BlueprintPure, Category = "SF|Stats")
	const FSFPlayerStats& GetStats() const { return Stats; }

	UFUNCTION(BlueprintPure, Category = "SF|Stats")
	float GetTotalDamageDealt() const { return Stats.TotalDamageDealt; }

	UFUNCTION(BlueprintPure, Category = "SF|Stats")
	int32 GetTotalDownedCount() const { return Stats.TotalDownedCount; }

	UFUNCTION(BlueprintPure, Category = "SF|Stats")
	int32 GetTotalReviveCount() const { return Stats.TotalReviveCount; }

	void CopyStatsFrom(const USFPlayerStatsComponent* Other);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SF|Stats")
	void ResetStats();

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "SF|Stats")
	FSFPlayerStats Stats;
};
