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

	void RequestPlayerSelectionChange(const APlayerState* RequestingPlayer, uint8 DesiredSlot);
	void SetHeroSelected(const APlayerState* SelectingPlayer, USFHeroDefinition* SelectedDefinition);
	bool IsSlotOccupied(uint8 SlotId) const;
	bool IsDefinitionSelected(const USFHeroDefinition* Definition) const;
	void SetHeroDeselected(const USFHeroDefinition* DefinitionToDeselect);

	const TArray<FSFPlayerSelectionInfo>& GetPlayerSelection() const;

	bool CanStartHeroSelection() const;
	bool CanStartMatch() const;

private:
	UFUNCTION()
	void OnRep_PlayerSelectionArray();

public:
	FOnPlayerSelectionUpdated OnPlayerSelectionUpdated;

private:
	UPROPERTY(ReplicatedUsing = OnRep_PlayerSelectionArray)
	TArray<FSFPlayerSelectionInfo> PlayerSelectionArray;
};
