#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SFLobbyGameMode.generated.h"

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
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	//~End of AGameModeBase interface

	/** PlayerSlots 배열 가져오기 */
	const TArray<ASFPlayerSlot*>& GetPlayerSlots() const { return PlayerSlots; }

	void OnPlayerReadyChanged(APlayerController* PC);
	void UpdateHeroDisplayForPlayer(APlayerController* PC);

private:
	/** 레벨에 배치된 PlayerSlot들을 찾아서 SlotID 순으로 정렬 */
	void SetupPlayerSlots();

	/** PlayerSlots가 준비될 때까지 대기 후 UpdatePlayerSlots 호출 */
	void WaitForPlayerSlotsAndUpdate();

	/** PlayerSlots 업데이트 (블루프린트의 UpdatePlayerSlots) */
	void UpdatePlayerSlots();

	/** PC가 이미 어떤 슬롯에 추가되어 있는지 확인 */
	bool IsPCAlreadyAdded(APlayerController* PC) const;

	/** 빈 슬롯 찾아서 PC 추가 */
	void AddPCToEmptySlot(APlayerController* PC);

	/** 로그아웃한 플레이어 제거 */
	void RemoveDisconnectedPlayers();

	/** Slot 할당 로직 내부에서 PlayerInfo 초기화 */
	void InitializePlayerInfoForSlot(APlayerController* PC, const ASFPlayerSlot* Slot) const;

private:
	UPROPERTY()
	TArray<TObjectPtr<APlayerController>> PCs;
	
	/** SlotID 순서대로 정렬된 PlayerSlot 배열 */
	UPROPERTY()
	TArray<TObjectPtr<ASFPlayerSlot>> PlayerSlots;

	/** PlayerSlot 초기화 완료 여부 */
	UPROPERTY()
	bool bSlotsInit;

	/** Delay 후 재귀 체크용 타이머 핸들 */
	FTimerHandle WaitForSlotsTimerHandle;
};
