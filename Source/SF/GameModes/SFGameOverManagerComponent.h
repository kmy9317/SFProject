#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Player/SFPlayerInfoTypes.h"
#include "SFGameOverManagerComponent.generated.h"

struct FSFPlayerDeadStateMessage;
struct FSFPlayerDownedStateMessage;
struct FSFGameOverMessage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameOverDelegate);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SF_API USFGameOverManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	USFGameOverManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "SF|GameOver")
	bool IsGameOver() const { return bGameOver; }

	UFUNCTION(BlueprintPure, Category = "SF|GameOver")
	const FSFGameOverResult& GetGameOverResult() const { return GameOverResult; }

	UFUNCTION(BlueprintPure, Category = "SF|GameOver")
	float GetRemainingLobbyTime() const;

	UFUNCTION(BlueprintPure, Category = "SF|GameOver")
	int32 GetReadyCount() const { return ReadyCount; }

	UFUNCTION(BlueprintPure, Category = "SF|GameOver")
	int32 GetTotalPlayerCount() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SF|GameOver")
	void TriggerGameOver();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SF|GameOver")
	void NotifyPlayerReadyForLobby(APlayerController* PC);
	
protected:
	UFUNCTION()
	void OnRep_bGameOver();

	UFUNCTION()
	void OnRep_GameOverResult();

	UFUNCTION()
	void OnRep_ReadyCount();

private:
	void OnPlayerDeadStateChanged(FGameplayTag Channel, const FSFPlayerDeadStateMessage& Message);
	void OnPlayerDownedStateChanged(FGameplayTag Channel, const FSFPlayerDownedStateMessage& Message);
    
	void ScheduleGameOverCheck();
	void CheckGameOverCondition();
	bool AreAllPlayersIncapacitated() const;

	void CollectAllPlayerStats();
	void CollectAndBroadcastStats();
	void StartLobbyCountdown();
	void TravelToLobby();

	void CleanupInvalidReadyPlayers();
	void CheckAllPlayersReady();

public:
	UPROPERTY(BlueprintAssignable, Category = "SF|Events")
	FOnGameOverDelegate OnGameOver;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "SF|GameOver")
	float GameOverCheckDelay = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|GameOver")
	float StatsDisplayDelay = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|GameOver")
	float LobbyTransitionDelay = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|GameOver")
	TSoftObjectPtr<UWorld> LobbyLevel;

private:
	UPROPERTY(ReplicatedUsing = OnRep_bGameOver)
	bool bGameOver = false;

	UPROPERTY(ReplicatedUsing = OnRep_GameOverResult)
	FSFGameOverResult GameOverResult;

	UPROPERTY(ReplicatedUsing = OnRep_ReadyCount)
	int32 ReadyCount = 0;

	// 로비 전환 대기 상태 (서버만 사용)
	bool bWaitingForLobbyTransition = false;

	UPROPERTY()
	TSet<TWeakObjectPtr<APlayerController>> ReadyPlayers;

	FTimerHandle GameOverCheckTimerHandle;
	FTimerHandle LobbyCountdownHandle;
	FTimerHandle StatsDelayHandle;
    
	FGameplayMessageListenerHandle DeadStateListenerHandle;
	FGameplayMessageListenerHandle DownedStateListenerHandle;
};
