#include "SFLobbyGameMode.h"

#include "SFLobbyGameState.h"
#include "SFLogChannels.h"
#include "Actors/SFPlayerSlot.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/SFPlayerInfoTypes.h"
#include "Player/Lobby/SFLobbyPlayerController.h"
#include "Player/Lobby/SFLobbyPlayerState.h"
#include "System/SFOSSGameInstance.h"
#include "UI/Lobby/SFLobbyWidget.h"

ASFLobbyGameMode::ASFLobbyGameMode()
{
	bUseSeamlessTravel = true;
}

void ASFLobbyGameMode::InitGameState()
{
	Super::InitGameState();

	LobbyGameState = GetGameState<ASFLobbyGameState>();
}

void ASFLobbyGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void ASFLobbyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	// 비밀번호 검증
	USFOSSGameInstance* OSSGI = GetGameInstance<USFOSSGameInstance>();
	if (OSSGI)
	{
		FString InputPassword = UGameplayStatics::ParseOption(Options, TEXT("Password"));
        
		if (!OSSGI->ValidateSessionPassword(InputPassword, ErrorMessage))
		{
			UE_LOG(LogSF, Warning, TEXT("[LobbyGameMode] PreLogin rejected: %s from %s"), *ErrorMessage, *Address);
			return;
		}
	}

	// 최대 인원 체크
	if (PCs.Num() >= MaxPlayerCount)
	{
		ErrorMessage = TEXT("ServerFull");
		return;
	}
	
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

void ASFLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	// Host(listen server)의 경우 BeginPlay 보다 먼저 호출
	Super::PostLogin(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	// PCs 배열에 추가
	PCs.AddUnique(NewPlayer);

	UpdatePlayerSlots();
	UpdateStartButtonState();
}

void ASFLobbyGameMode::HandleSeamlessTravelPlayer(AController*& Controller)
{
	Super::HandleSeamlessTravelPlayer(Controller);

	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC)
	{
		return;
	}

	// PostLogin과 동일한 로직
	PCs.AddUnique(PC);

	UpdatePlayerSlots();
	UpdateStartButtonState();
}

void ASFLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (APlayerController* ExitingPC = Cast<APlayerController>(Exiting))
	{
		// PCs 배열에서 제거
		PCs.Remove(ExitingPC);
		UpdatePlayerSlots();
	}
	UpdateStartButtonState();
}

void ASFLobbyGameMode::RegisterPlayerSlot(ASFPlayerSlot* NewSlot)
{
	if (!NewSlot || PlayerSlots.Contains(NewSlot))
	{
		return;
	}
	
	PlayerSlots.Add(NewSlot);

	PlayerSlots.Sort([](const ASFPlayerSlot& A, const ASFPlayerSlot& B)
	{
		return A.GetSlotID() < B.GetSlotID();
	});
	
	// 슬롯이 추가될 때마다 즉시 업데이트 시도
	UpdatePlayerSlots();
}

void ASFLobbyGameMode::UpdatePlayerSlots()
{
	// 1. 아직 자리가 없는(GameState에 등록 안 된) 플레이어들에게 빈 자리 번호표 배정
	AssignSlotsToNewPlayers();

	// 2. 배정받은 번호표에 맞는 슬롯 찾아서 앉히기 
	RefreshSlotVisuals();
    
	// 3. 나간 사람 정리
	RemoveDisconnectedPlayers();
}

void ASFLobbyGameMode::AssignSlotsToNewPlayers()
{
	if (!LobbyGameState)
	{
		return;
	}
	
	for (APlayerController* PC : PCs)
	{
		if (!PC || !PC->PlayerState)
		{
			continue;
		}

		// 이미 자리를 배정받았는지 확인
		if (FindSelectionForPC(PC))
		{
			continue;
		}
  
		// 사용 가능한 모든 SlotID 수집 (로딩된 슬롯들 기준)
		uint8 NewSlotID = 255;
		for (uint8 i = 0; i < MaxPlayerCount; i++) 
		{
			// GameState에서 i번 ID가 쓰고 있는지 확인
			if (!LobbyGameState->IsSlotOccupied(i))
			{
				NewSlotID = i;
				break;
			}
		}

		if (NewSlotID != 255)
		{
			LobbyGameState->AddPlayerSelection(PC->PlayerState, NewSlotID);
		}
	}
}

void ASFLobbyGameMode::RefreshSlotVisuals()
{
	if (!LobbyGameState)
	{
		return;
	}
	// 모든 플레이어를 순회하며 자기 자리를 찾아가게 함
	const TArray<FSFPlayerSelectionInfo>& Selections = LobbyGameState->GetPlayerSelections();
    
	for (const FSFPlayerSelectionInfo& Selection : Selections)
	{
		ASFPlayerSlot* Slot = FindSlotByID(Selection.GetPlayerSlot());
		if (!Slot || Slot->GetCurrentPC())
		{
			continue;
		}
        
		APlayerController* TargetPC = FindPCForSelection(Selection);
		if (TargetPC)
		{
			AddPlayerToSlot(TargetPC, Slot);
		}
	}
}

void ASFLobbyGameMode::AddPlayerToSlot(APlayerController* PC, ASFPlayerSlot* Slot)
{
	if (!Slot || !PC)
	{
		return;
	}

	// 현재 Slot에 PC 추가
	Slot->AddPlayer(PC);
	
	const FSFPlayerSelectionInfo* Selection = FindSelectionForPC(PC);
	if (Selection)
	{
		// 슬롯의 HeroDisplay 업데이트
		Slot->UpdatePlayerDisplay(Selection->GetHeroDefinition(), CreateDisplayInfoFromGameState(PC));
	}
}

void ASFLobbyGameMode::RemoveDisconnectedPlayers()
{
	if (!LobbyGameState)
	{
		return;
	}
	
	for (ASFPlayerSlot* Slot : PlayerSlots)
	{
		if (!Slot || !Slot->HasValidPC())
		{
			continue;
		}
		
		APlayerController* SlotPC = Slot->GetCurrentPC();
		if (PCs.Contains(SlotPC))
		{
			continue;
		}

		// 로그아웃한 플레이어 정리
		if (APlayerState* PS = SlotPC->GetPlayerState<APlayerState>())
		{
			LobbyGameState->RemovePlayerSelection(PS);  
		}
		Slot->RemovePlayer(SlotPC);
	}
}

void ASFLobbyGameMode::OnPlayerInfoChanged(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}
	UpdateHeroDisplayForPlayer(PC);
	
	UpdateStartButtonState();
}

void ASFLobbyGameMode::UpdateHeroDisplayForPlayer(APlayerController* PC)
{
	if (!PC || !LobbyGameState)
	{
		return;
	}
	// Slot 찾기
	for (ASFPlayerSlot* Slot : PlayerSlots)
	{
		if (!Slot || Slot->GetCurrentPC() != PC)
		{
			continue;
		}
		
		const FSFPlayerSelectionInfo* Selection = FindSelectionForPC(PC);
		if (Selection)
		{
			Slot->UpdatePlayerDisplay(Selection->GetHeroDefinition(), CreateDisplayInfoFromGameState(PC));
		}
		break;
	}
}

void ASFLobbyGameMode::UpdateStartButtonState()
{
	if (!LobbyGameState)
	{
		return;
	}
	bool bAllReady = LobbyGameState->AreAllPlayersReady();
    
	// Listen Server의 Host PC만 업데이트
	if (APlayerController* HostPC = GetWorld()->GetFirstPlayerController())
	{
		if (ASFLobbyPlayerController* LobbyPC = Cast<ASFLobbyPlayerController>(HostPC))
		{
			if (USFLobbyWidget* LobbyWidget = Cast<USFLobbyWidget>(LobbyPC->GetMenuWidget()))
			{
				LobbyWidget->SetAllPlayersReady(bAllReady);
			}
		}
	}
}

ASFPlayerSlot* ASFLobbyGameMode::FindSlotByID(uint8 SlotID) const
{
	for (ASFPlayerSlot* Slot : PlayerSlots)
	{
		if (Slot && Slot->GetSlotID() == SlotID)
		{
			return Slot;
		}
	}
	return nullptr;
}

const FSFPlayerSelectionInfo* ASFLobbyGameMode::FindSelectionForPC(APlayerController* PC) const
{
	if (!PC || !PC->PlayerState || !LobbyGameState)
	{
		return nullptr;
	}
    
	APlayerState* PS = PC->PlayerState;
	const TArray<FSFPlayerSelectionInfo>& Selections = LobbyGameState->GetPlayerSelections();
    
	return Selections.FindByPredicate([PS](const FSFPlayerSelectionInfo& Info)
	{
		return Info.IsForPlayer(PS);
	});
}

APlayerController* ASFLobbyGameMode::FindPCForSelection(const FSFPlayerSelectionInfo& Selection) const
{
	for (APlayerController* PC : PCs)
	{
		if (PC && PC->PlayerState && Selection.IsForPlayer(PC->PlayerState))
		{
			return PC;
		}
	}
	return nullptr;
}

FSFPlayerInfo ASFLobbyGameMode::CreateDisplayInfoFromGameState(APlayerController* PC) const
{
	FSFPlayerInfo DisplayInfo;
    
	if (!PC)
	{
		return DisplayInfo;
	}
    
	const FSFPlayerSelectionInfo* Selection = FindSelectionForPC(PC);
	if (Selection)
	{
		DisplayInfo.PC = PC;
		DisplayInfo.PS = PC->PlayerState;
		DisplayInfo.PlayerName = Selection->GetPlayerNickname();
		DisplayInfo.bReady = Selection->IsReady();
	}
    
	return DisplayInfo;
}
