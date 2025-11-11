#include "SFPlayerInfoTypes.h"

#include "GameFramework/PlayerState.h"

FSFPlayerSelectionInfo::FSFPlayerSelectionInfo()
	: Slot(GetInvalidSlot())
	, PlayerUniqueId(FUniqueNetIdRepl::Invalid())
	, PlayerNickname{}
	, HeroDefinition(nullptr)
	, bReady(false)  
{
}

FSFPlayerSelectionInfo::FSFPlayerSelectionInfo(uint8 InSlot, const APlayerState* InPlayerState)
	: Slot(InSlot)
	, HeroDefinition(nullptr)
	, bReady(false)
{
	if (InPlayerState)
	{
		PlayerUniqueId = InPlayerState->GetUniqueId();
		PlayerNickname = InPlayerState->GetPlayerName();
	}
}

bool FSFPlayerSelectionInfo::IsForPlayer(const APlayerState* PlayerState) const
{
	if (!PlayerState)
	{
		return false;
	}

#if WITH_EDITOR
	return PlayerState->GetPlayerName() == PlayerNickname;
#else
	return PlayerState->GetUniqueId() == GetPlayerUniqueId();
#endif
}

bool FSFPlayerSelectionInfo::IsValid() const
{
#if WITH_EDITOR
	return true;
#else
	if (!PlayerUniqueId.IsValid())
	{
		return false;
	}
	if (Slot == GetInvalidSlot())
	{
		return false;
	}
	// if (Slot >= LCNetStatics::GetPlayerCountPerTeam() * 2)
	// {
	// 	return false;
	// }
	return true;
#endif
}

uint8 FSFPlayerSelectionInfo::GetInvalidSlot()
{
	return uint8(255);
}
