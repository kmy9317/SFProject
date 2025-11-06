#include "SFLobbyPlayerState.h"

#include "Kismet/GameplayStatics.h"
#include "Character/Hero/SFHeroDefinition.h"
#include "GameModes/Lobby/SFLobbyGameState.h"
#include "Player/SFPlayerState.h"
#include "Net/UnrealNetwork.h"

ASFLobbyPlayerState::ASFLobbyPlayerState()
{
	bReplicates = true;
	SetNetUpdateFrequency(100.f);
}

void ASFLobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASFLobbyPlayerState, PlayerSelection);
}

void ASFLobbyPlayerState::BeginPlay()
{
	Super::BeginPlay();
	LobbyGameState = Cast<ASFLobbyGameState>(UGameplayStatics::GetGameState(this));

	if (LobbyGameState)
	{
		LobbyGameState->OnPlayerSelectionUpdated.AddUObject(this, &ThisClass::PlayerSelectionUpdated);
	}
}

void ASFLobbyPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	ASFPlayerState* NewPlayerState = Cast<ASFPlayerState>(PlayerState);
	if (NewPlayerState)
	{
		NewPlayerState->SetPlayerSelection(PlayerSelection);
	}
}

void ASFLobbyPlayerState::Server_SetSelectedHeroDefinition_Implementation(USFHeroDefinition* NewDefinition)
{
	if (!LobbyGameState)
	{
		return;
	}
	if (!NewDefinition)
	{
		return;
	}

	if (LobbyGameState->IsDefinitionSelected(NewDefinition))
	{
		return;
	}

	if (PlayerSelection.GetHeroDefinition())
	{
		LobbyGameState->SetHeroDeselected(PlayerSelection.GetHeroDefinition());
	}
	PlayerSelection.SetHeroDefinition(NewDefinition);
	LobbyGameState->SetHeroSelected(this, NewDefinition);
}

bool ASFLobbyPlayerState::Server_SetSelectedHeroDefinition_Validate(USFHeroDefinition* NewDefinition)
{
	return true;
}

void ASFLobbyPlayerState::PlayerSelectionUpdated(const TArray<FSFPlayerSelectionInfo>& NewPlayerSelections)
{
	for (const FSFPlayerSelectionInfo& NewPlayerSelection : NewPlayerSelections)
	{
		if (NewPlayerSelection.IsForPlayer(this))
		{
			PlayerSelection = NewPlayerSelection;
			return;
		}
	}
}
