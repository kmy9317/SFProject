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
	void ClearAndPopulateTeamSelectionSlots();
	void SlotSelected(uint8 NewSlotID);

	void ConfigureGameState();

	void UpdatePlayerSelectionDisplay(const TArray<FSFPlayerSelectionInfo>& PlayerSelections);

	UFUNCTION()
	void StartHeroSelectionButtonClicked();
	
	void SwitchToHeroSelection();
	void HeroDefinitionLoaded();

	void HeroSelected(UObject* SelectedUObject);

	void SpawnCharacterDisplay();
	void UpdateHeroDisplay(const FSFPlayerSelectionInfo& PlayerSelectionInfo);

	UFUNCTION()
	void StartMatchButtonClicked();

private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidgetSwitcher> MainSwitcher;

	UPROPERTY(meta=(BindWidget))	
	TObjectPtr<UWidget> TeamSelectionRoot;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> StartHeroSelectionButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UUniformGridPanel> TeamSelectionSlotGridPanel;

	UPROPERTY(EditDefaultsOnly, Category = "TeamSelection")
	TSubclassOf<USFTeamSelectionWidget> TeamSelectionWidgetClass;

	UPROPERTY()
	TArray<USFTeamSelectionWidget*> TeamSelectionSlots;

	UPROPERTY(meta=(BindWidget))	
	TObjectPtr<UWidget> HeroSelectionRoot;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTileView> HeroSelectionTileView;

	// TODO : 현재 캐릭터의 Ability 관련 정보 UI
	// UPROPERTY(meta = (BindWidget))
	// TObjectPtr<ULCAbilityListView> AbilityListView;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<USFPlayerTeamLayoutWidget> PlayerTeamLayoutWidget;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> StartMatchButton;
	
	UPROPERTY()
	TObjectPtr<ASFLobbyPlayerController> SFLobbyPlayerController;

	UPROPERTY()
	TObjectPtr<ASFLobbyPlayerState> SFLobbyPlayerState;

	UPROPERTY()
	TObjectPtr<ASFLobbyGameState> SFGameState;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Display")
	TSubclassOf<ASFHeroDisplay> HeroDisplayClass;

	UPROPERTY()
	TObjectPtr<ASFHeroDisplay> HeroDisplay;

	FTimerHandle ConfigureGameStateTimerHandle;
};
