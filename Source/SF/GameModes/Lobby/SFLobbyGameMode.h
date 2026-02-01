#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Player/SFPlayerInfoTypes.h"
#include "SFLobbyGameMode.generated.h"

class ASFLobbyGameState;
class ASFHeroDisplay;
class ASFPlayerSlot;
/**
 * 
 */
UCLASS()
class SF_API ASFLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASFLobbyGameMode();

	//~AGameModeBase interface
	virtual void InitGameState() override;
	virtual void BeginPlay() override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void HandleSeamlessTravelPlayer(AController*& Controller) override;
	virtual void Logout(AController* Exiting) override;
	//~End of AGameModeBase interface

	/** PlayerSlots 배열 가져오기 */
	const TArray<ASFPlayerSlot*>& GetPlayerSlots() const { return PlayerSlots; }
	
	void RegisterPlayerSlot(ASFPlayerSlot* NewSlot);
	
	void OnPlayerInfoChanged(APlayerController* PC);
	void UpdateHeroDisplayForPlayer(APlayerController* PC);

private:
	
	/** 로그인/로그아웃 된 플레이어를 PlayerSlots에 추가/제거 */
	void UpdatePlayerSlots();

	void AssignSlotsToNewPlayers();

	void RefreshSlotVisuals();
	
	/** 빈 슬롯 찾아서 PC 추가 */
	void AddPlayerToSlot(APlayerController* PC, ASFPlayerSlot* Slot);

	/** 로그아웃한 플레이어 제거 */
	void RemoveDisconnectedPlayers();

	/** 현재 플레이어 상태에 따른 Start 버튼 업데이트 */
	void UpdateStartButtonState();

	ASFPlayerSlot* FindSlotByID(uint8 SlotID) const;
	const FSFPlayerSelectionInfo* FindSelectionForPC(APlayerController* PC) const;
	APlayerController* FindPCForSelection(const FSFPlayerSelectionInfo& Selection) const;
	FSFPlayerInfo CreateDisplayInfoFromGameState(APlayerController* PC) const;

protected:
	// 플레이 최대 인원수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby Config")
	int32 MaxPlayerCount = 4;
	
private:
	UPROPERTY()
	TObjectPtr<ASFLobbyGameState> LobbyGameState;

	/** 접속한 PC 목록 관리 */
	UPROPERTY()
	TArray<TObjectPtr<APlayerController>> PCs;
	
	/** SlotID 순서대로 정렬된 PlayerSlot 배열 */
	UPROPERTY()
	TArray<TObjectPtr<ASFPlayerSlot>> PlayerSlots;

	
};
