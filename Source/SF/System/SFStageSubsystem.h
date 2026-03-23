#pragma once

#include "CoreMinimal.h"
#include "Data/SFStageInfo.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SFStageSubsystem.generated.h"

struct FStreamableHandle;

UENUM()
enum class ESFTravelType : uint8
{
	None,
	Hard,
	Seamless
};

DECLARE_MULTICAST_DELEGATE(FOnBundleLoadStarted);
DECLARE_MULTICAST_DELEGATE(FOnBundlesReady);

/**
 * 
 */
UCLASS(Config = Game)
class SF_API USFStageSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// ===== 스테이지 정보 접근 =====
    
	UFUNCTION(BlueprintCallable, Category = "SF|Stage")
	const FSFStageInfo& GetCurrentStageInfo() const { return CurrentStageInfo; }

	UFUNCTION(BlueprintCallable, Category = "SF|Stage")
	void SetCurrentStageInfo(const FSFStageInfo& NewStageInfo);

	UFUNCTION(BlueprintCallable, Category = "SF|Stage")
	void ResetStageInfo();

	// ===== 플레이어 수 =====
    
	UFUNCTION(BlueprintCallable, Category = "SF|Stage")
	void SetPlayerCount(int32 Count);

	UFUNCTION(BlueprintPure, Category = "SF|Stage")
	int32 GetPlayerCount() const { return PlayerCount; }

	// ===== DataTable 조회 =====
    
	UFUNCTION(BlueprintCallable, Category = "SF|Stage")
	FSFStageInfo GetStageInfoForLevel(const FString& LevelName) const;

	const FSFStageConfig* GetStageConfigForLevel(const FString& LevelName) const;

	// ===== 헬퍼 함수 =====
    
	UFUNCTION(BlueprintCallable, Category = "SF|Stage")
	bool IsBossStage() const { return CurrentStageInfo.IsBossStage(); }

	UFUNCTION(BlueprintCallable, Category = "SF|Stage")
	bool IsNormalStage() const { return CurrentStageInfo.IsNormalStage(); }

	UFUNCTION(BlueprintCallable, Category = "SF|Stage")
	int32 GetCurrentStageIndex() const { return CurrentStageInfo.StageIndex; }

	// Hard Travel용
	FOnBundleLoadStarted OnHardTravelBundleLoadStarted;
	FOnBundlesReady OnHardTravelBundlesReady;

	// Seamless Travel용
	FOnBundleLoadStarted OnSeamlessTravelBundleLoadStarted;
	FOnBundlesReady OnSeamlessTravelBundlesReady;

private:
	// Travel 감지 콜백
	void OnPreLoadMap(const FString& MapName);
	void OnSeamlessTravelStart(UWorld* CurrentWorld, const FString& LevelName);
	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);

	// 레벨 이름으로 스테이지 정보 업데이트
	void UpdateStageInfoFromLevel(const FString& LevelName);

	// DataTable 비동기 로드 완료 콜백
	void OnConfigTableLoaded();

	void UpdateAssetBundlesForLevel(const FString& LevelName);
	void BroadcastBundleLoadStarted();
	void BroadcastBundlesReady();
	void OnBundleLoadComplete();

private:
	UPROPERTY()
	FSFStageInfo CurrentStageInfo;

	int32 PlayerCount = 1;

	// 스테이지 설정 DataTable
	UPROPERTY(Config)
	TSoftObjectPtr<UDataTable> StageConfigTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CachedConfigTable;
	
	TSharedPtr<FStreamableHandle> ConfigTableLoadHandle;

	ESFTravelType CurrentTravelType = ESFTravelType::None;

	// 중복 번들 로드 방지
	FString LastProcessedLevelForBundles;
};
