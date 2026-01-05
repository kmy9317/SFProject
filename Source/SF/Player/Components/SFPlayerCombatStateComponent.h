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
	
	// 다운 상태 (부활 게이지 진행 중)
	UPROPERTY(BlueprintReadOnly)
	bool bIsDowned = false;

	// 완전히 죽은 상태
	UPROPERTY(BlueprintReadOnly)
	bool bIsDead = false;

	// 무력화 상태 (Dead 또는 Downed)
	bool IsIncapacitated() const { return bIsDead || bIsDowned; }
	
	bool operator!=(const FSFHeroCombatInfo& Other) const
	{
		return RemainingDownCount != Other.RemainingDownCount 
			|| bIsDowned != Other.bIsDowned
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
	bool IsIncapacitated() const { return CombatInfo.IsIncapacitated(); }
	
	UFUNCTION(BlueprintPure, Category = "SF|Combat")
	bool IsDead() const { return CombatInfo.bIsDead; }

	UFUNCTION(BlueprintPure, Category = "SF|Combat")
	bool IsDowned() const { return CombatInfo.bIsDowned; }

	// 게임 중 사망 설정
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SF|Combat")
	void SetIsDead(bool bNewIsDead);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SF|Combat")
	void SetIsDowned(bool bNewIsDowned);

	UFUNCTION(BlueprintCallable, Category = "SF|Combat")
	void DecrementDownCount();

	UFUNCTION(BlueprintCallable, Category = "SF|Combat")
	void ResetDownCount();
	
	// Seamless Travel 복원
	void RestoreCombatStateFromTravel(const FSFHeroCombatInfo& InCombatInfo);

	// 초기 데이터가 서버로부터 도착했는지 여부 
	bool HasReceivedInitialCombatInfo() const;
	void MarkInitialDataReceived();

protected:
	UFUNCTION()
	void OnRep_CombatInfo();

	// CombatInfo 변경 감지 후 브로드캐스트
	void BroadcastCombatInfoChanged();

	// UI용 Dead 상태 변경 시 별도 GMS broadcast
	void BroadcastDeadStateChangedForUI();

	// UI용 Down 상태 변경 시 별도 GMS broadcast
	void BroadcastDownedStateChangedForUI();

	// Dead 상태 변경 시 별도 GMS broadcast
	void BroadcastDeadStateChanged();

	// Down 상태 변경 시 별도 GMS broadcast
	void BroadcastDownedStateChanged();

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

	// CombatInfo 대상이 아니라 일반 변수로 변화 감지를 위해 사용하는 변수
	bool bLastKnownDownedState = false;
};
