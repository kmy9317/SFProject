#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Player/SFPlayerInfoTypes.h"
#include "SFLobbyPlayerState.generated.h"

class ASFLobbyGameState;
class USFCharacterDefinition;

/**
 * 
 */
UCLASS()
class SF_API ASFLobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ASFLobbyPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void CopyProperties(APlayerState* PlayerState) override;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetSelectedHeroDefinition(USFHeroDefinition* NewDefinition);
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_SetReady(bool bInReady);

	bool IsReady() const { return PlayerSelection.IsReady(); }
	const FSFPlayerSelectionInfo& GetPlayerSelection() const { return PlayerSelection; }

	FSFPlayerInfo CreateDisplayInfo() const;

private:
	void PlayerSelectionUpdated(const TArray<FSFPlayerSelectionInfo>& NewPlayerSelections);


private:
	UPROPERTY()
	FSFPlayerSelectionInfo PlayerSelection;

	UPROPERTY()
	TObjectPtr<ASFLobbyGameState> LobbyGameState;
};
