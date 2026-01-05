#include "SFLobbyGameMode.h"

#include "SFLobbyGameState.h"
#include "SFLogChannels.h"
#include "Actors/SFPlayerSlot.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/SFPlayerInfoTypes.h"
#include "Player/Lobby/SFLobbyPlayerController.h"
#include "Player/Lobby/SFLobbyPlayerState.h"
#include "UI/Lobby/SFLobbyWidget.h"

ASFLobbyGameMode::ASFLobbyGameMode()
{
	bUseSeamlessTravel = true;
	bSlotsInit = false;
}

void ASFLobbyGameMode::InitGameState()
{
	Super::InitGameState();

	LobbyGameState = GetGameState<ASFLobbyGameState>();
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

	// PCs 배열에 추가
	PCs.AddUnique(NewPlayer);
	WaitForPlayerSlotsAndUpdate();

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
	WaitForPlayerSlotsAndUpdate();
	
	UpdateStartButtonState();
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

	UpdateStartButtonState();
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
			AddConnectedPlayer(PC);
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

void ASFLobbyGameMode::AddConnectedPlayer(APlayerController* PC)
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
			// 현재 Slot이 관리중인 HeroDisplay 업데이트
			Slot->AddPlayer(PC);

			
			if (ASFLobbyPlayerState* LobbyPS = PC->GetPlayerState<ASFLobbyPlayerState>())
			{
				uint8 SlotID = Slot->GetSlotID();
				// GameState의 PlayerSelectionArray 업데이트
				if (LobbyGameState)
				{
					LobbyGameState->AddPlayerSelection(LobbyPS, SlotID);
				}
				// PlayerInfo 초기화 TODO : PlayerInfo 정보를 PlayerSelectionInfo로 통합 고려
				const FSFPlayerSelectionInfo& Selection = LobbyPS->GetPlayerSelection();
				Slot->UpdatePlayerDisplay(Selection.GetHeroDefinition(), LobbyPS->CreateDisplayInfo());
			}
			
			break;
		}
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

		// Logout 함수에서 Pcs 배열 목록에서 PC 제거 했으니 PCs 배열에 없는 경우 PlayerSelection 및 Slot에서 제거
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
