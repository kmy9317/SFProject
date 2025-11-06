#include "SFLobbyGameState.h"

#include "Net/UnrealNetwork.h"

void ASFLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, PlayerSelectionArray, COND_None, REPNOTIFY_Always);
}

void ASFLobbyGameState::RequestPlayerSelectionChange(const APlayerState* RequestingPlayer, uint8 DesiredSlot)
{
	if (!HasAuthority() || IsSlotOccupied(DesiredSlot))
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
		PlayerSelectionPtr->SetSlot(DesiredSlot);
	}
	else
	{
		PlayerSelectionArray.Add(FSFPlayerSelectionInfo(DesiredSlot, RequestingPlayer));
	}

	OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
}

void ASFLobbyGameState::SetHeroSelected(const APlayerState* SelectingPlayer, USFHeroDefinition* SelectedDefinition)
{
	if (IsDefinitionSelected(SelectedDefinition))
	{
		return;
	}

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

void ASFLobbyGameState::SetHeroDeselected(const USFHeroDefinition* DefinitionToDeselect)
{
	if (!DefinitionToDeselect)
	{
		return;
	}
	
	FSFPlayerSelectionInfo* FoundPlayerSelection = PlayerSelectionArray.FindByPredicate([&](const FSFPlayerSelectionInfo& PlayerSelection)
		{
			return PlayerSelection.GetHeroDefinition() == DefinitionToDeselect;
		}
	);

	if (FoundPlayerSelection)
	{
		FoundPlayerSelection->SetHeroDefinition(nullptr);
		OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
	}
}

const TArray<FSFPlayerSelectionInfo>& ASFLobbyGameState::GetPlayerSelection() const
{
	return PlayerSelectionArray;
}

bool ASFLobbyGameState::CanStartHeroSelection() const
{
	return PlayerSelectionArray.Num() == PlayerArray.Num();
}

bool ASFLobbyGameState::CanStartMatch() const
{
	for (const FSFPlayerSelectionInfo& PlayerSelection : PlayerSelectionArray)
	{
		if (!PlayerSelection.GetHeroDefinition())
		{
			return false;
		}
	}
	return true;
}

void ASFLobbyGameState::OnRep_PlayerSelectionArray()
{
	OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
}