#include "SFLobbyWidget.h"

#include "SFHeroEntryWidget.h"
#include "Character/Hero/SFHeroDefinition.h"
#include "Components/Button.h"
#include "Components/TileView.h"
#include "Engine/StreamableManager.h"
#include "GameModes/Lobby/SFLobbyGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Lobby/SFLobbyPlayerController.h"
#include "Player/Lobby/SFLobbyPlayerState.h"
#include "System/SFAssetManager.h"
#include "UI/Common/CommonButtonBase.h"

void USFLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ConfigureGameState();
	SFLobbyPlayerController = GetOwningPlayer<ASFLobbyPlayerController>();
	SFLobbyPlayerState = GetOwningPlayerState<ASFLobbyPlayerState>();
	Button_Start->OnButtonClickedDelegate.AddDynamic(this, &ThisClass::StartMatchButtonClicked);
	Button_Ready->OnButtonClickedDelegate.AddDynamic(this, &ThisClass::ReadyButtonClicked);

	Button_Ready->SetIsEnabled(false);
	
	USFAssetManager::Get().LoadHeroDefinitions(FStreamableDelegate::CreateUObject(this, &ThisClass::HeroDefinitionLoaded));

	if (HeroSelectionTileView)
	{
		HeroSelectionTileView->OnItemSelectionChanged().AddUObject(this, &USFLobbyWidget::HeroSelected);
	}

	if (UKismetSystemLibrary::IsServer(GetWorld()))
	{
		Button_Start->SetVisibility(ESlateVisibility::Visible);
	}
}

void USFLobbyWidget::ConfigureGameState()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	SFGameState = World->GetGameState<ASFLobbyGameState>();
	if (!SFGameState)
	{
		World->GetTimerManager().SetTimer(ConfigureGameStateTimerHandle, this, &ThisClass::ConfigureGameState, 1.f);
	}
	else
	{
		SFGameState->OnPlayerSelectionUpdated.AddUObject(this, &ThisClass::UpdatePlayerSelectionDisplay);
		UpdatePlayerSelectionDisplay(SFGameState->GetPlayerSelections());
	}
}

void USFLobbyWidget::UpdatePlayerSelectionDisplay(const TArray<FSFPlayerSelectionInfo>& PlayerSelections)
{
	// Refresh Hero Selection
	for (UUserWidget* HeroEntryAsWidget :HeroSelectionTileView->GetDisplayedEntryWidgets())
	{
		USFHeroEntryWidget* HeroEntryWidget = Cast<USFHeroEntryWidget>(HeroEntryAsWidget);
		if (HeroEntryWidget)
		{
			HeroEntryWidget->SetSelected(false);
		}
	}

	// Update Team Selection Slots
	for (const FSFPlayerSelectionInfo& PlayerSelection : PlayerSelections)
	{
		if (!PlayerSelection.IsValid())
		{
			continue;
		}

		// HeroDefintion과 연관된 HeroEntryWidget을 찾아서 선택된 상태로 업데이트
		USFHeroEntryWidget* SelectedEntry = HeroSelectionTileView->GetEntryWidgetFromItem<USFHeroEntryWidget>(PlayerSelection.GetHeroDefinition());
		if (SelectedEntry)
		{
			SelectedEntry->SetSelected(true);
		}
	}

	//  PlayerSelection 업데이트 시 Ready 버튼 상태도 업데이트
	UpdateReadyButtonEnabled(PlayerSelections);
}

void USFLobbyWidget::HeroDefinitionLoaded()
{
	TArray<USFHeroDefinition*> LoadedHeroDefinitions;
	if (USFAssetManager::Get().GetLoadedHeroDefinitions(LoadedHeroDefinitions))
	{
		HeroSelectionTileView->SetListItems(LoadedHeroDefinitions);
	}
}

void USFLobbyWidget::HeroSelected(UObject* SelectedUObject)
{
	if (!SFLobbyPlayerState)
	{
		return;
	}

	if (USFHeroDefinition* HeroDefinition = Cast<USFHeroDefinition>(SelectedUObject))
	{
		SFLobbyPlayerState->Server_SetSelectedHeroDefinition(HeroDefinition);
	}
}

void USFLobbyWidget::UpdateReadyButtonEnabled(const TArray<FSFPlayerSelectionInfo>& PlayerSelections) const
{
	if (!Button_Ready)
	{
		return;
	}
    
	const FSFPlayerSelectionInfo* MySelection = FindMySelection(PlayerSelections);
	bool bHasHero = MySelection && MySelection->GetHeroDefinition() != nullptr;
	Button_Ready->SetIsEnabled(bHasHero);
}

const FSFPlayerSelectionInfo* USFLobbyWidget::FindMySelection(const TArray<FSFPlayerSelectionInfo>& PlayerSelections) const
{
	if (!SFLobbyPlayerState)
	{
		return nullptr;
	}
	return PlayerSelections.FindByPredicate([this](const FSFPlayerSelectionInfo& Selection)
		{
			return Selection.IsForPlayer(SFLobbyPlayerState);
		});
}

bool USFLobbyWidget::HasSelectedHero() const
{
	if (!SFLobbyPlayerState)
	{
		return false;
	}
    
	const FSFPlayerSelectionInfo& Selection = SFLobbyPlayerState->GetPlayerSelection();
	return Selection.GetHeroDefinition() != nullptr;
}

void USFLobbyWidget::ReadyButtonClicked()
{
	if (SFLobbyPlayerController)
	{
		SFLobbyPlayerController->ToggleReady();
	}
}

void USFLobbyWidget::StartMatchButtonClicked()
{
	if (SFLobbyPlayerController)
	{
		SFLobbyPlayerController->Server_RequestStartMatch();
	}
}

