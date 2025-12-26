#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"
#include "TimerManager.h"
#include "SFPlayFabSubsystem.generated.h"

//=========================세이브 데이터(임시)===========================
USTRUCT(BlueprintType)
struct FPlayerStats
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	int32 Gold = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 Wrath = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 Pride = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 Lust = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 Sloth = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 Greed = 0;
};
//====================================================================

UCLASS()
class SF_API USFPlayFabSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	//======================세이브 데이터 관리(임시)==========================
	UFUNCTION(BlueprintCallable, Category="SF|PlayFab")
	void AddGold(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="SF|PlayFab")
	int32 GetGold() const { return PlayerStats.Gold; }

	UFUNCTION(BlueprintCallable, Category="SF|PlayFab")
	void SavePlayerData();

	UFUNCTION(BlueprintCallable, Category="SF|PlayFab")
	void LoadPlayerData();

	UFUNCTION()
	const FPlayerStats& GetPlayerStats() const { return PlayerStats; }

	// UI용: 복사본 getter (BP/UMG에서도 안전하게 사용)
	UFUNCTION(BlueprintCallable, Category="SF|PlayFab")
	FPlayerStats GetPlayerStatsCopy() const { return PlayerStats; }

	// UI용: 통째로 세팅 후 SavePlayerData 호출하면 됨
	UFUNCTION(BlueprintCallable, Category="SF|PlayFab")
	void SetPlayerStats(const FPlayerStats& NewStats);

	UFUNCTION(BlueprintCallable, Category="SF|PlayFab")
	void SetPermanentUpgradeLevels(int32 InWrath, int32 InPride, int32 InLust, int32 InSloth, int32 InGreed);

	UFUNCTION(BlueprintCallable, Category="SF|PlayFab")
	void SetGold(int32 NewGold);
	//====================================================================

	//========================업그레이드 데이터 서버 전송======================
	// "스테이지1-1 진입 시 1회 전송" 트리거 용도 (SFHero/GameMode 등에서 호출 가능)
	UFUNCTION(BlueprintCallable, Category="SF|PlayFab")
	void StartRetrySendPermanentUpgradeDataToServer();

	// 스테이지 재진입/메인메뉴 복귀 후 다시 보내야 할 때 리셋
	UFUNCTION(BlueprintCallable, Category="SF|PlayFab")
	void ResetPermanentUpgradeSendState();

	UFUNCTION(BlueprintCallable, Category="SF|PlayFab")
	bool HasLoadedPlayerData() const { return bHasLoadedPlayerData; }
	//====================================================================

private:
	FPlayerStats PlayerStats;
	bool bHasLoadedPlayerData = false;

	// "이번 판(스테이지 진입) 이미 전송했는가" 중복 방지
	bool bPermanentUpgradeSent = false;

	//===========================PlayFab 로그인============================
	void LoginToPlayFab();
	void OnLoginSuccess(const PlayFab::ClientModels::FLoginResult& Result);
	void OnLoginError(const PlayFab::FPlayFabCppError& Error);
	//====================================================================

	//========================저장 & 로드 콜백 함수==========================
	void OnDataSaved(const PlayFab::ClientModels::FUpdateUserDataResult& Result);
	void OnSaveError(const PlayFab::FPlayFabCppError& Error);
	void OnDataLoaded(const PlayFab::ClientModels::FGetUserDataResult& Result);
	//====================================================================

	//========================업그레이드 데이터 서버 전송======================
	void TrySendPermanentUpgradeDataToServer();

	FTimerHandle RetrySendUpgradeTimerHandle;
	int32 RetrySendUpgradeAttempts = 0;
	static constexpr int32 MaxRetrySendUpgradeAttempts = 30;
	//====================================================================
};
