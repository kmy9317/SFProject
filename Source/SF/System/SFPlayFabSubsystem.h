#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/PlayFabClientAPI.h"
#include "Core/PlayFabClientDataModels.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
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
	//====================================================================

private:
	FPlayerStats PlayerStats;

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
};
