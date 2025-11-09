#include "SFLobbyPlayerController.h"

#include "SFLobbyPlayerState.h"
#include "SFLogChannels.h"
#include "GameModes/Lobby/SFLobbyGameMode.h"
#include "System/SFGameInstance.h"

ASFLobbyPlayerController::ASFLobbyPlayerController()
{
	bAutoManageActiveCameraTarget = false;
	bReady = false;
}

void ASFLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	Server_UpdatePlayerInfo();
}

void ASFLobbyPlayerController::Server_RequestStartMatch_Implementation()
{
	USFGameInstance* GameInstance = GetGameInstance<USFGameInstance>();
	if (GameInstance)
	{
		GameInstance->StartMatch();
	}
}

bool ASFLobbyPlayerController::Server_RequestStartMatch_Validate()
{
	return true;
}

void ASFLobbyPlayerController::UpdatePlayerInfo()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 0.1초 후 서버 RPC 호출
	FTimerHandle DelayTimerHandle;
	World->GetTimerManager().SetTimer(
		DelayTimerHandle,
		this,
		&ASFLobbyPlayerController::Server_UpdatePlayerInfo,
		0.1f,
		false
	);
}

void ASFLobbyPlayerController::Server_UpdatePlayerInfo_Implementation()
{
	ASFLobbyGameMode* LobbyGM = GetWorld()->GetAuthGameMode<ASFLobbyGameMode>();
	if (!LobbyGM)
	{
		return;
	}
	
	LobbyGM->UpdatePlayerInfo(this);
}

void ASFLobbyPlayerController::Server_ToggleReadyStatus_Implementation()
{
	ASFLobbyPlayerState* LobbyPS = GetPlayerState<ASFLobbyPlayerState>();
	if (!LobbyPS)
	{
		return;
	}
    
	const FSFPlayerSelectionInfo& Selection = LobbyPS->GetPlayerSelection();
	if (!Selection.GetHeroDefinition())
	{
		UE_LOG(LogSF, Warning, 
			TEXT("[Server] Player %s tried to ready without hero (possible exploit attempt)"), 
			*LobbyPS->GetPlayerName()
		);
		return;
	}
	
	PlayerInfo.bReady = !PlayerInfo.bReady;

	if (ASFLobbyGameMode* LobbyGM = GetWorld()->GetAuthGameMode<ASFLobbyGameMode>())
	{
		LobbyGM->UpdatePlayerInfo(this);
	}
}
