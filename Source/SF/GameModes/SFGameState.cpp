#include "SFGameState.h"
#include "SFPortalManagerComponent.h"

ASFGameState::ASFGameState()
{
	// Portal Manager 생성
	PortalManager = CreateDefaultSubobject<USFPortalManagerComponent>(TEXT("PortalManager"));
}

void ASFGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	if (PlayerState)
	{
		// 델리게이트를 브로드캐스트하여 UI를 포함한 모든 리스너에게 새 플레이어가 추가되었음을 알림
		OnPlayerAdded.Broadcast(PlayerState);
	}
}

void ASFGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);

	if (PlayerState)
	{
		// 델리게이트를 브로드캐스트하여 UI를 포함한 모든 리스너에게 플레이어가 제거되었음을 알림
		OnPlayerRemoved.Broadcast(PlayerState);
	}
}
