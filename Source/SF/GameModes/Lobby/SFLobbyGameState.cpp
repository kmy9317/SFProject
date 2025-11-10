#include "SFLobbyGameState.h"

#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

void ASFLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, PlayerSelectionArray, COND_None, REPNOTIFY_Always);
}

void ASFLobbyGameState::UpdatePlayerSelection(const APlayerState* RequestingPlayer, uint8 InSlot)
{
	if (!HasAuthority() || IsSlotOccupied(InSlot))
	{
		return;
	}

	FSFPlayerSelectionInfo* PlayerSelectionPtr = PlayerSelectionArray.FindByPredicate([&](const FSFPlayerSelectionInfo& PlayerSelection)
		{
			return PlayerSelection.IsForPlayer(RequestingPlayer);
		}
	);

	if (PlayerSelectionPtr)
	{
		PlayerSelectionPtr->SetSlot(InSlot);
	}
	else
	{
		PlayerSelectionArray.Add(FSFPlayerSelectionInfo(InSlot, RequestingPlayer));
	}

	OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
}

void ASFLobbyGameState::RemovePlayerSelection(const APlayerState* LeavingPlayer)
{
	if (!HasAuthority() || !LeavingPlayer)
	{
		return;
	}

	int32 RemovedIndex = PlayerSelectionArray.IndexOfByPredicate([&](const FSFPlayerSelectionInfo& Selection)
	{
		// TODO : Editor에서는 PlayerName으로 Player를 찾는데(중복 될 수 있음)실제 패키징시 UniqueID로 찾음
		return Selection.IsForPlayer(LeavingPlayer);
	});

	if (RemovedIndex != INDEX_NONE)
	{
		PlayerSelectionArray.RemoveAt(RemovedIndex);
		OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
	}
}

void ASFLobbyGameState::SetHeroSelected(const APlayerState* SelectingPlayer, USFHeroDefinition* SelectedDefinition)
{
	// Hero 중복 선택 불가 
	// if (IsDefinitionSelected(SelectedDefinition))
	// {
	// 	return;
	// }

	FSFPlayerSelectionInfo* FoundPlayerSelection = PlayerSelectionArray.FindByPredicate([&](const FSFPlayerSelectionInfo& PlayerSelection)
		{
			return PlayerSelection.IsForPlayer(SelectingPlayer);
		}
	);

	if (FoundPlayerSelection)
	{
		FoundPlayerSelection->SetHeroDefinition(SelectedDefinition);
		OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
	}
}

void ASFLobbyGameState::SetHeroDeselected(const APlayerState* Player)
{
	FSFPlayerSelectionInfo* FoundPlayerSelection = PlayerSelectionArray.FindByPredicate([&](const FSFPlayerSelectionInfo& Selection)
		{
			return Selection.IsForPlayer(Player);
		}
	);

	if (FoundPlayerSelection)
	{
		FoundPlayerSelection->SetHeroDefinition(nullptr);
		OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
	}
}

bool ASFLobbyGameState::IsSlotOccupied(uint8 SlotId) const
{
	for (const FSFPlayerSelectionInfo& PlayerSelection : PlayerSelectionArray)
	{
		if (PlayerSelection.GetPlayerSlot() == SlotId)
		{
			return true;
		}
	}

	return false;
}

bool ASFLobbyGameState::IsDefinitionSelected(const USFHeroDefinition* Definition) const
{
	const FSFPlayerSelectionInfo* FoundPlayerSelection = PlayerSelectionArray.FindByPredicate([&](const FSFPlayerSelectionInfo& PlayerSelection)
		{
			return PlayerSelection.GetHeroDefinition() == Definition;
		}
	);

	return FoundPlayerSelection != nullptr;

}

const TArray<FSFPlayerSelectionInfo>& ASFLobbyGameState::GetPlayerSelections() const
{
	return PlayerSelectionArray;
}

void ASFLobbyGameState::SetPlayerReady(const APlayerState* Player, bool bReady)
{
	FSFPlayerSelectionInfo* Selection = PlayerSelectionArray.FindByPredicate(
		[&](const FSFPlayerSelectionInfo& Info) {
			return Info.IsForPlayer(Player);
		}
	);

	if (Selection)
	{
		Selection->SetReady(bReady);
		OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
	}
}

bool ASFLobbyGameState::AreAllPlayersReady() const
{
	if (PlayerSelectionArray.Num() == 0)
	{
		return false;
	}
	for (const FSFPlayerSelectionInfo& Selection : PlayerSelectionArray)
	{
		if (!Selection.IsReady())
			return false;
	}

	return true;
}

void ASFLobbyGameState::OnRep_PlayerSelectionArray()
{
	OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
}
