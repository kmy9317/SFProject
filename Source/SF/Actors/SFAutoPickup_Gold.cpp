#include "SFAutoPickup_Gold.h"
#include "Player/SFPlayerState.h"

void ASFAutoPickup_Gold::ApplyCollectEffect(ASFPlayerState* PlayerState, int32 CollectAmount)
{
	if (PlayerState)
	{
		PlayerState->AddGold(CollectAmount);
	}
}
