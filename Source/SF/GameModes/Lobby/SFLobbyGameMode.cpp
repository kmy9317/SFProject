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
				// GameState의 PlayerSelectionArray에서 제거
				if (APlayerState* PS = ValidPC->GetPlayerState<APlayerState>())
				{
					// PlayerSelectionArray에서 제거
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

void ASFLobbyGameMode::UpdatePlayerInfo(APlayerController* RequestingPC)
{
	if (!RequestingPC)
	{
		return;
	}
	for (ASFPlayerSlot* Slot : PlayerSlots)
	{
		if (!Slot)
		{
			continue;
		}

		// Selected Character = HeroDisplay의 CachedPC
		ASFHeroDisplay* HeroDisplay = Slot->GetHeroDisplay();
		if (!HeroDisplay)
		{
			continue;
		}

		// HeroDisplay가 표시 중인 PC와 요청한 PC가 같은지 확인
		if (Slot->GetCurrentPC() == RequestingPC)
		{
			// 플레이어가 선택한 Hero 가져오기
			USFHeroDefinition* SelectedHero = nullptr;
			if (ASFLobbyPlayerState* SFPS = RequestingPC->GetPlayerState<ASFLobbyPlayerState>())
			{
				SelectedHero = const_cast<USFHeroDefinition*>(SFPS->GetPlayerSelection().GetHeroDefinition());
			}
            
			// 현재 표시 중인 Hero와 다르면 업데이트
			if (SelectedHero && SelectedHero != Slot->GetCurrentHeroDefinition())
			{
				Slot->SwitchHeroDefinition(SelectedHero);
			}
			
			// Get Player Name
			FString PlayerName = TEXT("");
			if (APlayerState* PS = RequestingPC->GetPlayerState<APlayerState>())
			{
				PlayerName = PS->GetPlayerName();
			}

			ASFLobbyPlayerController* LobbyPC = Cast<ASFLobbyPlayerController>(RequestingPC);
			if (!LobbyPC)
			{
				continue;
			}

			FSFPlayerInfo NewPlayerInfo;
			NewPlayerInfo.PC = RequestingPC;
			NewPlayerInfo.PlayerName = PlayerName;
			NewPlayerInfo.bReady = LobbyPC->PlayerInfo.bReady;
			NewPlayerInfo.PS = RequestingPC->PlayerState;

			HeroDisplay->UpdatePlayerInfo(NewPlayerInfo);
			CheckAllPlayersReady();
			return;
		}
	}

	ASFLobbyPlayerController* LobbyPC = Cast<ASFLobbyPlayerController>(RequestingPC);
	if (!LobbyPC)
	{
		return;
	}

	LobbyPC->UpdatePlayerInfo();
}

void ASFLobbyGameMode::CheckAllPlayersReady()
{
	bool bAllPlayersReady = true;

	for (APlayerController* PC : PCs)
	{
		ASFLobbyPlayerController* LobbyPC = Cast<ASFLobbyPlayerController>(PC);
		if (!LobbyPC)
		{
			continue;
		}

		if (!LobbyPC->PlayerInfo.bReady)
		{
			bAllPlayersReady = false;
			break;
		}
	}

	// 호스트 PC 찾기(Listen Server에서만 가능)
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	ASFLobbyPlayerController* LobbyPC = Cast<ASFLobbyPlayerController>(PC);
	if (LobbyPC)
	{
		if (USFLobbyWidget* LobbyWidget = Cast<USFLobbyWidget>(LobbyPC->GetMenuWidget()))
		{
			LobbyWidget->bAllPlayersReady = bAllPlayersReady;
		}
	}
}
