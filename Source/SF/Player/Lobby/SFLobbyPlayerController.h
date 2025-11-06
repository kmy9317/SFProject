#pragma once

#include "CoreMinimal.h"
#include "SFMenuPlayerController.h"
#include "SFLobbyPlayerController.generated.h"

DECLARE_DELEGATE(FOnSwitchToHeroSelection);

/**
 * 
 */
UCLASS()
class SF_API ASFLobbyPlayerController : public ASFMenuPlayerController
{
	GENERATED_BODY()

public:
	ASFLobbyPlayerController();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestPlayerSelectionChange(uint8 NewSlotID);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_StartHeroSelection();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestStartMatch();
	
	UFUNCTION(Client, Reliable)
	void Client_StartHeroSelection();

public:
	FOnSwitchToHeroSelection OnSwitchToHeroSelection;
};
