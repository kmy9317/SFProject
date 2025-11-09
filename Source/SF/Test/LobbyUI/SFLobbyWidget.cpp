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

void USFLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ConfigureGameState();
	SFLobbyPlayerController = GetOwningPlayer<ASFLobbyPlayerController>();
	Button_Start->OnClicked.AddDynamic(this, &ThisClass::StartMatchButtonClicked);
	Button_Ready->OnClicked.AddDynamic(this, &ThisClass::ReadyButtonClicked);

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
	UpdateReadyButtonEnabled();
}

void USFLobbyWidget::HeroDefinitionLoaded()
{
	TArray<USFHeroDefinition*> LoadedCharacterDefinitions;
	if (USFAssetManager::Get().GetLoadedHeroDefinitions(LoadedCharacterDefinitions))
	{
		HeroSelectionTileView->SetListItems(LoadedCharacterDefinitions);
	}
}

void USFLobbyWidget::HeroSelected(UObject* SelectedUObject)
{
	if (!SFLobbyPlayerState)
	{
		SFLobbyPlayerState = GetOwningPlayerState<ASFLobbyPlayerState>();
	}

	if (!SFLobbyPlayerState)
	{
		return;
	}

	if (USFHeroDefinition* CharacterDefinition = Cast<USFHeroDefinition>(SelectedUObject))
	{
		SFLobbyPlayerState->Server_SetSelectedHeroDefinition(CharacterDefinition);
	}
}

void USFLobbyWidget::UpdateReadyButtonEnabled() const
{
	if (!Button_Ready)
	{
		return;
	}
    
	// Hero 선택 여부에 따라 버튼 활성화/비활성화
	bool bHasHero = HasSelectedHero();
	Button_Ready->SetIsEnabled(bHasHero);
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
		SFLobbyPlayerController->Server_ToggleReadyStatus();
	}
}

void USFLobbyWidget::StartMatchButtonClicked()
{
	if (SFLobbyPlayerController)
	{
		SFLobbyPlayerController->Server_RequestStartMatch();
	}
}

