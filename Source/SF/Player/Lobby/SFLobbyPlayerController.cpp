#include "SFLobbyPlayerController.h"

#include "SFLobbyPlayerState.h"
#include "System/SFGameInstance.h"

ASFLobbyPlayerController::ASFLobbyPlayerController()
{
	bAutoManageActiveCameraTarget = false;
}

void ASFLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();
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

void ASFLobbyPlayerController::ToggleReady()
{
	ASFLobbyPlayerState* LobbyPS = GetPlayerState<ASFLobbyPlayerState>();
	if (!LobbyPS)
	{
		return;
	}

	bool bNewReady = !LobbyPS->IsReady();
	LobbyPS->Server_SetReady(bNewReady);
}
