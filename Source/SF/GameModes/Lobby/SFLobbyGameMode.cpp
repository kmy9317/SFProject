#include "SFLobbyGameMode.h"

#include "SFLobbyGameState.h"
#include "SFLogChannels.h"
#include "Actors/SFHeroDisplay.h"
#include "Actors/SFPlayerSlot.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/SFPlayerInfoTypes.h"
#include "Player/Lobby/SFLobbyPlayerController.h"
#include "Player/Lobby/SFLobbyPlayerState.h"
#include "Test/LobbyUI/SFLobbyWidget.h"

ASFLobbyGameMode::ASFLobbyGameMode()
{
	bUseSeamlessTravel = true;
	bSlotsInit = false;
}

void ASFLobbyGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	// PlayerSlots 초기화
	SetupPlayerSlots();
}

void ASFLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	PCs.AddUnique(NewPlayer);
	WaitForPlayerSlotsAndUpdate();
}

void ASFLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (APlayerController* ExitingPC = Cast<APlayerController>(Exiting))
	{
		// PCs 배열에서 제거
		PCs.Remove(ExitingPC);
		WaitForPlayerSlotsAndUpdate();
	}
	
}

void ASFLobbyGameMode::SetupPlayerSlots()
{
	if (bSlotsInit)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// === 1. 레벨의 모든 PlayerSlot 찾기 ===
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, ASFPlayerSlot::StaticClass(), FoundActors);

	if (FoundActors.Num() == 0)
	{
		UE_LOG(LogSF, Warning, TEXT("[LobbyGameMode] No PlayerSlots found in level!"));
		return;
	}

	// === 2. ASFPlayerSlot으로 캐스팅 ===
	TArray<ASFPlayerSlot*> UnsortedSlots;
	for (AActor* Actor : FoundActors)
	{
		if (ASFPlayerSlot* Slot = Cast<ASFPlayerSlot>(Actor))
		{
			UnsortedSlots.Add(Slot);
		}
	}

	// === 3. SlotID 기준으로 오름차순 정렬 ===
	UnsortedSlots.Sort([](const ASFPlayerSlot& A, const ASFPlayerSlot& B)
	{
		return A.GetSlotID() < B.GetSlotID();
	});

	// === 4. PlayerSlots 배열에 저장 ===
	PlayerSlots = UnsortedSlots;

	bSlotsInit = true;
}

void ASFLobbyGameMode::WaitForPlayerSlotsAndUpdate()
{
	if (bSlotsInit)
	{
		UpdatePlayerSlots();
	}
	else
	{
		UWorld* World = GetWorld();
		if (!World)
		{
			return;
		}

		// 기존 타이머 클리어
		World->GetTimerManager().ClearTimer(WaitForSlotsTimerHandle);

		// 0.05초 후 다시 WaitForPlayerSlotsAndUpdate 호출
		World->GetTimerManager().SetTimer(
			WaitForSlotsTimerHandle,
			this,
			&ASFLobbyGameMode::WaitForPlayerSlotsAndUpdate,
			0.05f,
			false // 반복 X (재귀적으로 호출)
		);
	}
}

void ASFLobbyGameMode::UpdatePlayerSlots()
{
	for (APlayerController* PC : PCs)
	{
		if (!PC)
		{
			continue;
		}

		// PC가 이미 Slot에 추가되어 있는지 확인 
		bool bPCAlreadyAdded = IsPCAlreadyAdded(PC);

		// PCAlreadyAdded가 false면 빈 슬롯에 추가 
		if (!bPCAlreadyAdded)
		{
			AddPCToEmptySlot(PC);
		}
	}

	// 로그아웃한 플레이어를 PlayerSlot에서 제거 
	RemoveDisconnectedPlayers();
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

void ASFLobbyGameMode::AddPCToEmptySlot(APlayerController* PC)
{
	// PlayerSlots를 순회하면서 빈 슬롯 찾기
	for (ASFPlayerSlot* Slot : PlayerSlots)
	{
		if (!Slot)
		{
			continue;
		}

		// 빈 슬롯 찾기(낮은 SlotID가 높은 SlotID보다 먼저 채워짐)
		if (!Slot->GetCurrentPC())
		{
			// Slot Display 업데이트
			Slot->AddPlayer(PC);

			if (ASFLobbyGameState* LobbyGS = GetGameState<ASFLobbyGameState>())
			{
				if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
				{
					uint8 SlotID = Slot->GetSlotID();
					// GameState의 PlayerSelectionArray 업데이트
					LobbyGS->UpdatePlayerSelection(PS, SlotID);
				}
			}

			// PlayerInfo 초기화
			InitializePlayerInfoForSlot(PC, Slot);
			break;
		}
	}
}

void ASFLobbyGameMode::RemoveDisconnectedPlayers()
{
	ASFLobbyGameState* LobbyGS = GetGameState<ASFLobbyGameState>();
	if (!LobbyGS)
	{
		return;
	}
	
	for (ASFPlayerSlot* Slot : PlayerSlots)
	{
		if (!Slot)
		{
			continue;
		}

		if (Slot->HasValidPC())
		{
			APlayerController* ValidPC = Slot->GetCurrentPC();
			if (!PCs.Contains(ValidPC))
			{
				if (APlayerState* PS = ValidPC->GetPlayerState<APlayerState>())
				{
					const TArray<FSFPlayerSelectionInfo>& Selections = LobbyGS->GetPlayerSelections();
					for (const FSFPlayerSelectionInfo& Selection : Selections)
					{
						if (Selection.IsForPlayer(PS))
						{
							// GameState의 PlayerSelectionArray에서 제거
							LobbyGS->RemovePlayerSelection(PS);
							break;
						}
					}
				}
				// Slot에서 PC, Definition 제거
				Slot->RemovePlayer(ValidPC);
			}
		}
	}
}

void ASFLobbyGameMode::InitializePlayerInfoForSlot(APlayerController* PC, const ASFPlayerSlot* Slot) const
{
	ASFLobbyPlayerState* LobbyPS = PC->GetPlayerState<ASFLobbyPlayerState>();
	if (!LobbyPS)
	{
		UE_LOG(LogSF, Warning, TEXT("[InitializePlayerInfoForSlot] No LobbyPlayerState"));
		return;
	}

	ASFHeroDisplay* HeroDisplay = Slot->GetHeroDisplay();
	if (!HeroDisplay)
	{
		return;
	}

	// PlayerState에서 데이터 가져오기
	const FSFPlayerSelectionInfo& Selection = LobbyPS->GetPlayerSelection();

	FSFPlayerInfo InitialPlayerInfo;
	InitialPlayerInfo.PC = PC;
	InitialPlayerInfo.PS = PC->PlayerState;
	InitialPlayerInfo.PlayerName = Selection.GetPlayerNickname();
	InitialPlayerInfo.bReady = Selection.IsReady();  // 초기에는 false

	// 현재 클라이언트의 PlayerInfo 기반 HeroDisplay의 Widget 업데이트
	HeroDisplay->UpdatePlayerInfo(InitialPlayerInfo);
}

void ASFLobbyGameMode::OnPlayerReadyChanged(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}
	UpdateHeroDisplayForPlayer(PC);
    
	// Start 버튼 활성화 체크
	if (ASFLobbyGameState* LobbyGS = GetGameState<ASFLobbyGameState>())
	{
		bool bAllReady = LobbyGS->AreAllPlayersReady();
        
		// Listen Server의 Host UI 업데이트
		if (APlayerController* HostPC = GetWorld()->GetFirstPlayerController())
		{
			if (ASFLobbyPlayerController* LobbyPC = Cast<ASFLobbyPlayerController>(HostPC))
			{
				if (USFLobbyWidget* LobbyWidget = Cast<USFLobbyWidget>(LobbyPC->GetMenuWidget()))
				{
					LobbyWidget->bAllPlayersReady = bAllReady;
				}
			}
		}
	}
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
			continue;

		ASFHeroDisplay* HeroDisplay = Slot->GetHeroDisplay();
		if (!HeroDisplay)
			continue;

		const FSFPlayerSelectionInfo& Selection = LobbyPS->GetPlayerSelection();

		// Hero 변경 확인
		if (Selection.GetHeroDefinition() != Slot->GetCurrentHeroDefinition())
		{
			Slot->SwitchHeroDefinition(const_cast<USFHeroDefinition*>(Selection.GetHeroDefinition()));
		}
		
		FSFPlayerInfo DisplayInfo;
		DisplayInfo.PC = PC;
		DisplayInfo.PS = PC->PlayerState;
		DisplayInfo.PlayerName = Selection.GetPlayerNickname();
		DisplayInfo.bReady = Selection.IsReady();

		HeroDisplay->UpdatePlayerInfo(DisplayInfo);
		break;
	}
}
