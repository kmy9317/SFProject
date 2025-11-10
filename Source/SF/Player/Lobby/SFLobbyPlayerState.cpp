#include "SFLobbyPlayerState.h"

#include "Kismet/GameplayStatics.h"
#include "Character/Hero/SFHeroDefinition.h"
#include "GameModes/Lobby/SFLobbyGameMode.h"
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

void ASFLobbyPlayerState::Server_SetReady_Implementation(bool bInReady)
{
	if (!PlayerSelection.GetHeroDefinition())
	{
		return;
	}

	// Ready 상태 변경
	PlayerSelection.SetReady(bInReady);

	// GameState에 알림 
	if (ASFLobbyGameState* LobbyGS = GetWorld()->GetGameState<ASFLobbyGameState>())
	{
		LobbyGS->SetPlayerReady(this, bInReady);
	}

	// GameMode에 알림 (HeroDisplay 업데이트용) TODO ; GameState에서 일괄 처리?
	if (ASFLobbyGameMode* LobbyGM = GetWorld()->GetAuthGameMode<ASFLobbyGameMode>())
	{
		LobbyGM->OnPlayerReadyChanged(GetPlayerController());
	}
}

void ASFLobbyPlayerState::Server_SetSelectedHeroDefinition_Implementation(USFHeroDefinition* NewDefinition)
{
	if (!LobbyGameState || !NewDefinition)
	{
		return;
	}

	// 현재 같은 Hero를 선택한 경우 처리x
	if (PlayerSelection.GetHeroDefinition() == NewDefinition)
	{
		return;
	}

	// Hero 중복 선택 불가 
	// if (LobbyGameState->IsDefinitionSelected(NewDefinition))
	// {
	// 	return;
	// }
	
	LobbyGameState->SetHeroDeselected(this);
	PlayerSelection.SetHeroDefinition(NewDefinition);
	LobbyGameState->SetHeroSelected(this, NewDefinition);

	// UpdateHeroDisplayForPlayer 호출 TODO : GameState에서 일괄 처리?
	if (ASFLobbyGameMode* LobbyGM = GetWorld()->GetAuthGameMode<ASFLobbyGameMode>())
	{
		LobbyGM->UpdateHeroDisplayForPlayer(GetPlayerController());
	}
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

void ASFLobbyPlayerState::OnRep_PlayerSelection(FSFPlayerSelectionInfo OldPlayerSelection)
{
	
}
