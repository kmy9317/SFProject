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

void ASFLobbyPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (LobbyGameState)
	{
		LobbyGameState->OnPlayerSelectionUpdated.RemoveAll(this);
	}
	
	Super::EndPlay(EndPlayReason);
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

	if (!LobbyGameState)
	{
		return;
	}

	// GameState에 알림(현재 LobbyGS에서 일괄적으로 접속한 클라이언트들의 정보 broadcast)
	LobbyGameState->SetPlayerReady(this, bInReady);

	// GameMode에 알림 (HeroDisplay 업데이트용) TODO ; 리스너가 3개 이상 필요하면 GameState 델리게이트 패턴 고려
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
	
	//LobbyGameState->SetHeroDeselected(this);
	LobbyGameState->SetHeroSelected(this, NewDefinition);

	// 내부적으로 HeroDisplay 업데이트 호출 TODO : 리스너가 3개 이상 필요하면 GameState 델리게이트 패턴 고려
	if (ASFLobbyGameMode* LobbyGM = GetWorld()->GetAuthGameMode<ASFLobbyGameMode>())
	{
		LobbyGM->OnPlayerReadyChanged(GetPlayerController());
	}
}

bool ASFLobbyPlayerState::Server_SetSelectedHeroDefinition_Validate(USFHeroDefinition* NewDefinition)
{
	return true;
}

FSFPlayerInfo ASFLobbyPlayerState::CreateDisplayInfo() const
{
	FSFPlayerInfo DisplayInfo;
	DisplayInfo.PC = GetPlayerController();
	DisplayInfo.PS = const_cast<APlayerState*>(static_cast<const APlayerState*>(this));
	DisplayInfo.PlayerName = PlayerSelection.GetPlayerNickname();
	DisplayInfo.bReady = PlayerSelection.IsReady();
	return DisplayInfo;
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

