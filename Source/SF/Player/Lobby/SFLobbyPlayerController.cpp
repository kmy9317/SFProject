#include "SFLobbyPlayerController.h"

#include "SFLobbyPlayerState.h"
#include "System/SFGameInstance.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"

ASFLobbyPlayerController::ASFLobbyPlayerController()
{
	bAutoManageActiveCameraTarget = false;
}

void ASFLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 1. "LobbyCam" 태그가 달린 액터 검색
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("LobbyCam"), FoundActors);

	if (FoundActors.Num() > 0)
	{
		AActor* TargetCamera = FoundActors[0];

		// 시점(View Target)을 카메라로 고정, blendTime 0.0f로 즉시 전환
		SetViewTargetWithBlend(TargetCamera, 0.0f);

		UE_LOG(LogTemp, Log, TEXT("Camera Set directly to LobbyCam."));		
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to find Actor with tag 'LobbyCam'. Check your Level"));
	}
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
