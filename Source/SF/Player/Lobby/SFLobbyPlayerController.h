#pragma once

#include "CoreMinimal.h"
#include "SFMenuPlayerController.h"
#include "Player/SFPlayerInfoTypes.h"
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

	/** 플레이어 정보 업데이트 요청 (클라이언트 → 서버) */
	UFUNCTION(BlueprintCallable, Category = "PlayerInfo")
	void UpdatePlayerInfo();

	/** Ready 상태 */
	UFUNCTION(BlueprintCallable, Category = "PlayerInfo")
	void SetReady(bool bNewReady) { bReady = bNewReady; }
	
	UFUNCTION(BlueprintPure, Category = "PlayerInfo")
	bool IsReady() const { return bReady; }

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_ToggleReadyStatus();

protected:
	virtual void BeginPlay() override;


public:
	FSFPlayerInfo PlayerInfo;
	
private:
	/** 서버 RPC: 플레이어 정보 업데이트 */
	UFUNCTION(Server, Reliable)
	void Server_UpdatePlayerInfo();

	/** Ready 상태 */
	UPROPERTY()
	bool bReady;
};
