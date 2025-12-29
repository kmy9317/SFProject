#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
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

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SF|GameOver")
	void TriggerGameOver();

protected:
	UFUNCTION()
	void OnRep_bGameOver();

private:
	void OnPlayerDeadStateChanged(FGameplayTag Channel, const FSFPlayerDeadStateMessage& Message);
	void OnPlayerDownedStateChanged(FGameplayTag Channel, const FSFPlayerDownedStateMessage& Message);
    
	void ScheduleGameOverCheck();
	void CheckGameOverCondition();
	bool AreAllPlayersIncapacitated() const;

public:
	UPROPERTY(BlueprintAssignable, Category = "SF|Events")
	FOnGameOverDelegate OnGameOver;

private:
	UPROPERTY(ReplicatedUsing = OnRep_bGameOver)
	bool bGameOver = false;

	UPROPERTY(EditDefaultsOnly, Category = "SF|GameOver")
	float GameOverCheckDelay = 0.3f;

	FTimerHandle GameOverCheckTimerHandle;
    
	FGameplayMessageListenerHandle DeadStateListenerHandle;
	FGameplayMessageListenerHandle DownedStateListenerHandle;
};
