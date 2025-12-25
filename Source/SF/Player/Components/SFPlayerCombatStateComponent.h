#pragma once

#include "CoreMinimal.h"
#include "Components/PlayerStateComponent.h"
#include "SFPlayerCombatStateComponent.generated.h"

USTRUCT(BlueprintType)
struct FSFHeroCombatInfo
{
	GENERATED_BODY()

	// 남은 다운 가능 횟수
	UPROPERTY(BlueprintReadOnly)
	int32 RemainingDownCount = 3;

	// 다른 플레이어 부활시킨 횟수
	UPROPERTY(BlueprintReadOnly)
	int32 ReviveCount = 0;

	// 완전히 죽은 상태
	UPROPERTY(BlueprintReadOnly)
	bool bIsDead = false;
	
	bool operator!=(const FSFHeroCombatInfo& Other) const
	{
		return RemainingDownCount != Other.RemainingDownCount 
			|| ReviveCount != Other.ReviveCount
			|| bIsDead != Other.bIsDead;
	}

	bool operator==(const FSFHeroCombatInfo& Other) const
	{
		return !(*this != Other);
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeroCombatInfoChanged, const FSFHeroCombatInfo&, CombatInfo);


/**
 * 플레이어의 전투 관련 상태를 관리하는 컴포넌트
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFPlayerCombatStateComponent : public UPlayerStateComponent
{
	GENERATED_BODY()

public:
	USFPlayerCombatStateComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "SF|Combat")
	static USFPlayerCombatStateComponent* FindPlayerCombatStateComponent(const AActor* Actor);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	UFUNCTION(BlueprintPure, Category = "SF|Combat")
	const FSFHeroCombatInfo& GetCombatInfo() const { return CombatInfo; }

	UFUNCTION(BlueprintPure, Category = "SF|Combat")
	float GetInitialReviveGauge() const;
    
	UFUNCTION(BlueprintPure, Category = "SF|Combat")
	int32 GetRemainingDownCount() const { return CombatInfo.RemainingDownCount; }

	UFUNCTION(BlueprintPure, Category = "SF|Combat")
	int32 GetReviveCount() const { return CombatInfo.ReviveCount; }

	UFUNCTION(BlueprintPure, Category = "SF|Combat")
	bool IsDead() const { return CombatInfo.bIsDead; }

	UFUNCTION(BlueprintPure, Category = "SF|Combat")
	bool IsDowned() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SF|Combat")
	void SetIsDead(bool bNewIsDead);

	UFUNCTION(BlueprintCallable, Category = "SF|Combat")
	void DecrementDownCount();

	UFUNCTION(BlueprintCallable, Category = "SF|Combat")
	void ResetDownCount();

	UFUNCTION(BlueprintCallable, Category = "SF|Combat")
	void IncrementReviveCount();

	void SetCombatInfoFromTravel(const FSFHeroCombatInfo& InCombatInfo);

	// 초기 데이터가 서버로부터 도착했는지 여부 
	bool HasReceivedInitialCombatInfo() const;
	void MarkInitialDataReceived();

protected:
	UFUNCTION()
	void OnRep_CombatInfo();

	// 변경 감지 후 브로드캐스트
	void BroadcastCombatInfoChanged();

	// Dead 상태 변경 시 별도 GMS broadcast
	void BroadcastDeadStateChanged();

public:
	UPROPERTY(BlueprintAssignable, Category = "SF|Combat")
	FOnHeroCombatInfoChanged OnCombatInfoChanged;
	
protected:

	// RemainingDownCount별 초기 ReviveGauge 값 (index 0 = 첫 다운)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Combat")
	TArray<float> InitialReviveGaugeByDownCount = { 80.f, 55.f, 30.f };

	// 초기 다운 가능 횟수
	UPROPERTY(VisibleDefaultsOnly, Category = "SF|Combat")
	int32 InitialDownCount = 3;
	
	UPROPERTY(ReplicatedUsing = OnRep_CombatInfo, BlueprintReadOnly, Category = "SF|Combat")
	FSFHeroCombatInfo CombatInfo;

private:
	// 변경 감지용 캐시
	FSFHeroCombatInfo CachedCombatInfo;

	// 초기 복제 완료 플래그
	bool bHasReceivedInitialCombatInfo = false;
};
