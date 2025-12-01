#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFLobbyWidget.generated.h"

class USFPlayerTeamLayoutWidget;
class USFAbilityListView;
class ASFHeroDisplay;
class ASFLobbyPlayerState;
class UTileView;
struct FSFPlayerSelectionInfo;
class ASFLobbyGameState;
class ASFLobbyPlayerController;
class USFTeamSelectionWidget;
class UUniformGridPanel;
class UButton;
class UCommonButtonBase;
class UWidgetSwitcher;
class UWidget;

/**
 * 
 */
UCLASS()
class SF_API USFLobbyWidget : public UUserWidget
{
	GENERATED_BODY()
	public:
	virtual void NativeConstruct() override;

private:
	void ConfigureGameState();

	void UpdatePlayerSelectionDisplay(const TArray<FSFPlayerSelectionInfo>& PlayerSelections);
	
	void HeroDefinitionLoaded();
	void HeroSelected(UObject* SelectedUObject);

	void UpdateReadyButtonEnabled(const TArray<FSFPlayerSelectionInfo>& PlayerSelections) const;

	const FSFPlayerSelectionInfo* FindMySelection(const TArray<FSFPlayerSelectionInfo>& PlayerSelections) const;

	bool HasSelectedHero() const;

	UFUNCTION()
	void ReadyButtonClicked();
	
	UFUNCTION()
	void StartMatchButtonClicked();

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bAllPlayersReady = false;

private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidgetSwitcher> MainSwitcher;

	UPROPERTY(meta=(BindWidget))	
	TObjectPtr<UWidget> HeroSelectionRoot;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTileView> HeroSelectionTileView;

	// TODO : 현재 캐릭터의 Ability 관련 정보 UI
	// UPROPERTY(meta = (BindWidget))
	// TObjectPtr<ULCAbilityListView> AbilityListView;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCommonButtonBase> Button_Start;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCommonButtonBase> Button_Ready;

	UPROPERTY()
	TObjectPtr<ASFLobbyPlayerController> SFLobbyPlayerController;

	UPROPERTY()
	TObjectPtr<ASFLobbyPlayerState> SFLobbyPlayerState;

	UPROPERTY()
	TObjectPtr<ASFLobbyGameState> SFGameState;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Display")
	TSubclassOf<ASFHeroDisplay> HeroDisplayClass;

	FTimerHandle ConfigureGameStateTimerHandle;
};
