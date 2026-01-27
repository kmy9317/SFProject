// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Player/SFPlayerInfoTypes.h"
#include "SFLobbyGameState.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerSelectionUpdated, const TArray<FSFPlayerSelectionInfo>& /*NewPlayerSelection*/);

class USFHeroDefinition;

/**
 * 
 */
UCLASS()
class SF_API ASFLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void AddPlayerSelection(const APlayerState* RequestingPlayer, uint8 InSlot);
	void RemovePlayerSelection(const APlayerState* LeavingPlayer);
	void SetHeroSelected(const APlayerState* SelectingPlayer, USFHeroDefinition* SelectedDefinition);
	void SetHeroDeselected(const APlayerState* Player);
	bool IsSlotOccupied(uint8 SlotId) const;
	bool IsDefinitionSelected(const USFHeroDefinition* Definition) const;

	const TArray<FSFPlayerSelectionInfo>& GetPlayerSelections() const;

	void SetPlayerReady(const APlayerState* Player, bool bReady);
	bool AreAllPlayersReady() const;

private:
	UFUNCTION()
	void OnRep_PlayerSelectionArray();

public:
	// 선택 정보 변경시 브로드캐스트 (서버/클라이언트 모두)
	FOnPlayerSelectionUpdated OnPlayerSelectionUpdated;

private:
	// 모든 플레이어의 선택 정보
	UPROPERTY(ReplicatedUsing = OnRep_PlayerSelectionArray)
	TArray<FSFPlayerSelectionInfo> PlayerSelectionArray;
};
