// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SFGameState.generated.h"

class USFPortalManagerComponent;
class ASFPortal;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStateChangedDelegate, APlayerState*, PlayerState);

/**
 * 
 */
UCLASS()
class SF_API ASFGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ASFGameState();
	
	/** Portal Manager 가져오기 */
	UFUNCTION(BlueprintPure, Category = "SF|GameState")
	USFPortalManagerComponent* GetPortalManager() const { return PortalManager; }

protected:

	//~AGameStateBase interface
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	//~End of AGameStateBase interface

public:
	/** 새 플레이어가 PlayerArray에 추가될 때 브로드캐스트. */
	UPROPERTY(BlueprintAssignable, Category = "SF|Events")
	FOnPlayerStateChangedDelegate OnPlayerAdded;

	/** 플레이어가 PlayerArray에서 제거될 때 브로드캐스트 */
	UPROPERTY(BlueprintAssignable, Category = "SF|Events")
	FOnPlayerStateChangedDelegate OnPlayerRemoved;
	
private:
	/** Portal 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SF|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USFPortalManagerComponent> PortalManager;
};
