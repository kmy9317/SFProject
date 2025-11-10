#pragma once

#include "CoreMinimal.h"
#include "SFMenuPlayerController.h"
#include "SFLobbyPlayerController.generated.h"

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
	void Server_RequestStartMatch();

	UFUNCTION(BlueprintCallable)
	void ToggleReady();

protected:
	virtual void BeginPlay() override;

};
