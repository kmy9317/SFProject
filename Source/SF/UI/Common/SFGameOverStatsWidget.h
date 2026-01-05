#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "SFGameOverStatsWidget.generated.h"

struct FSFLobbyReadyMessage;
class UButton;
class USFPlayerStatsEntryWidget;
struct FSFGameOverResult;
class UVerticalBox;
class UTextBlock;
struct FSFPlayerGameStats;
/**
 * 
 */
UCLASS()
class SF_API USFGameOverStatsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USFGameOverStatsWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void OnGameOverStatsReceived(FGameplayTag Channel, const FSFGameOverResult& Result);
	void OnLobbyReadyCountReceived(FGameplayTag Channel, const FSFLobbyReadyMessage& Message);

	void InitializeStats(const FSFGameOverResult& Result);
	void DisplayPlayerStats(const TArray<FSFPlayerGameStats>& PlayerStats);
    
	float GetRemainingTime() const;
	void UpdateCountdownDisplay();
	void UpdateReadyCountDisplay(int32 InReadyCount, int32 InTotalCount);

	UFUNCTION()
	void OnReadyButtonClicked();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StageNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> StatsContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountdownText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ReadyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ReadyCountText;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USFPlayerStatsEntryWidget> PlayerStatsEntryClass;

private:
	FGameplayMessageListenerHandle StatsListenerHandle;
	FGameplayMessageListenerHandle ReadyCountListenerHandle;

	float TargetLobbyTime = 0.f;
	bool bCountdownActive = false;
	bool bIsReady = false;
};
