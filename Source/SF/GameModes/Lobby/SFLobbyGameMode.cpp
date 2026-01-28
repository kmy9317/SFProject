#include "SFLobbyGameMode.h"

#include "SFLobbyGameState.h"
#include "Actors/SFPlayerSlot.h"
#include "GameFramework/PlayerState.h"
#include "Player/SFPlayerInfoTypes.h"
#include "Player/Lobby/SFLobbyPlayerController.h"
#include "Player/Lobby/SFLobbyPlayerState.h"
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
		if (!PC)
		{
			continue;
		}
		ASFLobbyPlayerState* LobbyPS = PC->GetPlayerState<ASFLobbyPlayerState>();
		if (!LobbyPS)
		{
			continue;
		}
		
		// 이미 자리를 배정받았는지 GameState를 통해 확인
		bool bAlreadyAssigned = false;
		const TArray<FSFPlayerSelectionInfo>& Selections = LobbyGameState->GetPlayerSelections();
		for (const FSFPlayerSelectionInfo& Info : Selections)
		{
			if (Info.IsForPlayer(LobbyPS)) // 이미 배정된 기록이 있음
			{
				bAlreadyAssigned = true;
				break;
			}
		}

		if (bAlreadyAssigned)
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
			LobbyGameState->AddPlayerSelection(LobbyPS, NewSlotID);
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
		uint8 TargetSlotID = Selection.GetPlayerSlot(); // 내 자리 번호
        
		// 로딩된 슬롯들 중에서 내 번호(TargetSlotID)를 가진 슬롯 찾기
		ASFPlayerSlot* MySlot = nullptr;
		for (ASFPlayerSlot* Slot : PlayerSlots)
		{
			if (Slot->GetSlotID() == TargetSlotID)
			{
				MySlot = Slot;
				break;
			}
		}

		// 슬롯을 찾았고, 그 슬롯에 아직 PC가 안 앉아있다면 착석
		if (MySlot && !MySlot->GetCurrentPC())
		{
			APlayerController* TargetPC = nullptr;
			for (APlayerController* PC : PCs)
			{
				if (PC && PC->PlayerState && Selection.IsForPlayer(PC->PlayerState))
				{
					TargetPC = PC;
					break;
				}
			}
            
			if (TargetPC)
			{
				AddPlayerToSlot(TargetPC, MySlot);
			}
		}
	}
}

bool ASFLobbyGameMode::IsPCAlreadyAdded(APlayerController* PC) const
{
	// === PlayerSlots를 순회하면서 해당 PC가 있는지 확인 ===
	for (ASFPlayerSlot* Slot : PlayerSlots)
	{
		if (!Slot)
		{
			continue;
		}

		if (Slot->GetCurrentPC() == PC)
		{
			return true;
		}
	}

	return false;
}

void ASFLobbyGameMode::AddPlayerToSlot(APlayerController* PC, ASFPlayerSlot* Slot)
{
	if (!Slot || !PC)
	{
		return;
	}

	// 현재 Slot이 관리중인 HeroDisplay 업데이트
	Slot->AddPlayer(PC);
	
	if (ASFLobbyPlayerState* LobbyPS = PC->GetPlayerState<ASFLobbyPlayerState>())
	{
		// PlayerInfo 초기화 TODO : PlayerInfo 정보를 PlayerSelectionInfo로 통합 고려
		const FSFPlayerSelectionInfo& Selection = LobbyPS->GetPlayerSelection();
		Slot->UpdatePlayerDisplay(Selection.GetHeroDefinition(), LobbyPS->CreateDisplayInfo());
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
		
		APlayerController* ValidPC = Slot->GetCurrentPC();

		// Logout 함수의 Pcs 배열 목록에서 PC 제거 했으니 PCs 배열에 없는 경우 PlayerSelection 및 Slot에서 제거
		if (!PCs.Contains(ValidPC))
		{
			if (APlayerState* PS = ValidPC->GetPlayerState<APlayerState>())
			{
				LobbyGameState->RemovePlayerSelection(PS);  
			}
			// Slot에서 PC 제거
			Slot->RemovePlayer(ValidPC);
		}
	}
}

void ASFLobbyGameMode::OnPlayerReadyChanged(APlayerController* PC)
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
	ASFLobbyPlayerState* LobbyPS = PC->GetPlayerState<ASFLobbyPlayerState>();
	if (!LobbyPS)
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
		
		const FSFPlayerSelectionInfo& Selection = LobbyPS->GetPlayerSelection();
		Slot->UpdatePlayerDisplay(Selection.GetHeroDefinition(), LobbyPS->CreateDisplayInfo());
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
