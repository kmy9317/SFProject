#include "SFLobbyWidget.h"

#include "SFHeroEntryWidget.h"
#include "SFNetStatics.h"
#include "SFPlayerTeamLayoutWidget.h"
#include "SFTeamSelectionWidget.h"
#include "Actors/SFHeroDisplay.h"
#include "Character/Hero/SFHeroDefinition.h"
#include "Components/Button.h"
#include "Components/TileView.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/PlayerStart.h"
#include "GameModes/Lobby/SFLobbyGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Lobby/SFLobbyPlayerController.h"
#include "Player/Lobby/SFLobbyPlayerState.h"
#include "System/SFAssetManager.h"

void USFLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ClearAndPopulateTeamSelectionSlots();
	ConfigureGameState();
	SFLobbyPlayerController = GetOwningPlayer<ASFLobbyPlayerController>();
	if (SFLobbyPlayerController)
	{
		SFLobbyPlayerController->OnSwitchToHeroSelection.BindUObject(this, &USFLobbyWidget::SwitchToHeroSelection);
	}
	StartHeroSelectionButton->SetIsEnabled(false);
	StartHeroSelectionButton->OnClicked.AddDynamic(this, &ThisClass::StartHeroSelectionButtonClicked);
	StartMatchButton->SetIsEnabled(false);
	StartMatchButton->OnClicked.AddDynamic(this, &ThisClass::StartMatchButtonClicked);
	
	USFAssetManager::Get().LoadCharacterDefinitions(FStreamableDelegate::CreateUObject(this, &ThisClass::HeroDefinitionLoaded));

	if (HeroSelectionTileView)
	{
		HeroSelectionTileView->OnItemSelectionChanged().AddUObject(this, &USFLobbyWidget::HeroSelected);
	}

	SpawnCharacterDisplay();
}

void USFLobbyWidget::ClearAndPopulateTeamSelectionSlots()
{
	TeamSelectionSlotGridPanel->ClearChildren();

	// TODO : 해당 로직 수정 필요
	for (int i = 0; i < USFNetStatics::GetPlayerCountPerTeam() * 2; i++)
	{
		USFTeamSelectionWidget* NewSelectionWidget = CreateWidget<USFTeamSelectionWidget>(this, TeamSelectionWidgetClass);
		if (NewSelectionWidget)
		{
			NewSelectionWidget->SetSlotID(i);
			UUniformGridSlot* NewGridSlot = TeamSelectionSlotGridPanel->AddChildToUniformGrid(NewSelectionWidget);
			if (NewGridSlot)
			{
				int32 Row = i % USFNetStatics::GetPlayerCountPerTeam();
				int32 Column = i < USFNetStatics::GetPlayerCountPerTeam() ? 0 : 1;
				NewGridSlot->SetRow(Row);
				NewGridSlot->SetColumn(Column);
			}
			NewSelectionWidget->OnSlotClicked.AddUObject(this, &ThisClass::SlotSelected);
			TeamSelectionSlots.Add(NewSelectionWidget);
		}
	}
}

void USFLobbyWidget::SlotSelected(uint8 NewSlotID)
{
	if (SFLobbyPlayerController)
	{
		SFLobbyPlayerController->Server_RequestPlayerSelectionChange(NewSlotID);
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
		UpdatePlayerSelectionDisplay(SFGameState->GetPlayerSelection());
	}
}

void USFLobbyWidget::UpdatePlayerSelectionDisplay(const TArray<FSFPlayerSelectionInfo>& PlayerSelections)
{
	for (USFTeamSelectionWidget* TeamSelectionSlot : TeamSelectionSlots)
	{
		TeamSelectionSlot->UpdateSlotInfo("Empty");
	}

	for (UUserWidget* CharacterEntryAsWidget :HeroSelectionTileView->GetDisplayedEntryWidgets())
	{
		USFHeroEntryWidget* CharacterEntryWidget = Cast<USFHeroEntryWidget>(CharacterEntryAsWidget);
		if (CharacterEntryWidget)
		{
			CharacterEntryWidget->SetSelected(false);
		}
	}

	for (const FSFPlayerSelectionInfo& PlayerSelection : PlayerSelections)
	{
		if (!PlayerSelection.IsValid())
		{
			continue;
		}
		TeamSelectionSlots[PlayerSelection.GetPlayerSlot()]->UpdateSlotInfo(PlayerSelection.GetPlayerNickname());

		USFHeroEntryWidget* SelectedEntry = HeroSelectionTileView->GetEntryWidgetFromItem<USFHeroEntryWidget>(PlayerSelection.GetHeroDefinition());
		if (SelectedEntry)
		{
			SelectedEntry->SetSelected(true);
		}

		// TODO : 다른 플레이어의 캐릭터도 보여주도록 제거 or 수정
		if (PlayerSelection.IsForPlayer(GetOwningPlayerState()))
		{
			UpdateHeroDisplay(PlayerSelection);
		}
	}

	if (SFGameState)
	{
		StartHeroSelectionButton->SetIsEnabled(SFGameState->CanStartHeroSelection());
		StartMatchButton->SetIsEnabled(SFGameState->CanStartMatch());
	}

	if (PlayerTeamLayoutWidget)
	{
		PlayerTeamLayoutWidget->UpdatePlayerSelection(PlayerSelections);
	}
}

void USFLobbyWidget::StartHeroSelectionButtonClicked()
{
	if (SFLobbyPlayerController)
	{
		SFLobbyPlayerController->Server_StartHeroSelection();
	}
}

void USFLobbyWidget::SwitchToHeroSelection()
{
	MainSwitcher->SetActiveWidget(HeroSelectionRoot);
}

void USFLobbyWidget::HeroDefinitionLoaded()
{
	TArray<USFHeroDefinition*> LoadedCharacterDefinitions;
	if (USFAssetManager::Get().GetLoadedCharacterDefinitions(LoadedCharacterDefinitions))
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

void USFLobbyWidget::SpawnCharacterDisplay()
{
	if (HeroDisplay)
	{
		return;
	}
	if (!HeroDisplayClass)
	{
		return;
	}
	
	FTransform CharacterDisplayTransform = FTransform::Identity;

	// TODO : 접속한 플레이어별 PlayerStart 위치 지정 필요
	AActor* PlayerStart = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass());
	if (PlayerStart)
	{
		CharacterDisplayTransform = PlayerStart->GetActorTransform();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	HeroDisplay = GetWorld()->SpawnActor<ASFHeroDisplay>(HeroDisplayClass, CharacterDisplayTransform, SpawnParams);
	GetOwningPlayer()->SetViewTarget(HeroDisplay);
}

void USFLobbyWidget::UpdateHeroDisplay(const FSFPlayerSelectionInfo& PlayerSelectionInfo)
{
	if (!PlayerSelectionInfo.GetHeroDefinition())
	{
		return;
	}

	HeroDisplay->ConfigureWithHeroDefination(PlayerSelectionInfo.GetHeroDefinition());

	// TODO : CharacterInfo or PawnData 에서 캐릭터에 부여되는 Ability를 가져와서 AbilityListView에 적용되도록 구조 생각
	//AbilityListView->ConfigureAbilities();
}

void USFLobbyWidget::StartMatchButtonClicked()
{
	if (SFLobbyPlayerController)
	{
		SFLobbyPlayerController->Server_RequestStartMatch();
	}
}
