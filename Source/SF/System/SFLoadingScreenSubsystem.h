// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SFLoadingScreenSubsystem.generated.h"

struct FStreamableHandle;
/**
 * 맵별 로딩 스크린 설정
 */
USTRUCT(BlueprintType)
struct FSFMapLoadingConfig : public FTableRowBase
{
	GENERATED_BODY()
    
	/** Seamless Travel용 로딩 스크린 위젯 (CommonLoadingScreen) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading")
	TSoftClassPtr<UUserWidget> SeamlessLoadingWidget;
    
	/** Hard Travel용 로딩 스크린 위젯 (MoviePlayer) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading")
	TSoftClassPtr<UUserWidget> HardLoadingWidget;
    
	/** 로딩 스크린 최소 표시 시간 (Hard Travel) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading")
	float MinimumDisplayTime = 2.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPreLoadingScreenWidgetChangedDelegate, TSubclassOf<UUserWidget>, NewWidgetClass);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLoadingScreenWidgetChangedDelegate, TSubclassOf<UUserWidget>, NewWidgetClass);

/**
 * 
 */
UCLASS(config = Game)
class SF_API USFLoadingScreenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	/** 
	 * 다음 레벨의 로딩 스크린 미리 로드 
	 * @param NextLevelName 이동할 레벨 이름
	 * @note ServerTravel 호출 전에 호출 필요 (스테이지 클리어 시점 등)
	 */
	UFUNCTION(BlueprintCallable, Category = "SF|Loading")
	void PreloadLoadingScreenForLevel(const FString& NextLevelName);
	
	// Hard Travel 로딩 스크린 위젯 설정
	UFUNCTION(BlueprintCallable, Category = "SF|Loading")
	void SetHardLoadingScreenContentWidget(TSubclassOf<UUserWidget> NewWidgetClass);

	// 현재 설정된 Hard Travel 로딩 스크린 위젯 클래스 리턴
	UFUNCTION(BlueprintPure, Category = "SF|Loading")
	TSubclassOf<UUserWidget> GetHardLoadingScreenContentWidget() const;
	
	// Seamless Travel 로딩 스크린 위젯 설정
	UFUNCTION(BlueprintCallable, Category = "SF|Loading")
	void SetSeamlessLoadingScreenContentWidget(TSubclassOf<UUserWidget> NewWidgetClass);

	// 현재 설정된 Seamless Travel 로딩 스크린 위젯 클래스 리턴
	UFUNCTION(BlueprintPure, Category = "SF|Loading")
	TSubclassOf<UUserWidget> GetSeamlessLoadingScreenContentWidget() const;
	
	// 수동으로 로딩 스크린 시작 
	UFUNCTION(BlueprintCallable, Category = "SF|Loading")
	void StartHardLoadingScreen();
	
	// 수동으로 로딩 스크린 종료
	UFUNCTION(BlueprintCallable, Category = "SF|Loading")
	void StopHardLoadingScreen();

private:
	// Seamless Travel 시작 감지 
	void OnSeamlessTravelStart(UWorld* CurrentWorld, const FString& LevelName);
	
	// 하드 트래블 감지용 (OpenLevel)
	void OnPreLoadMap(const FString& MapName);

	// 맵 로딩 완료 감지
	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);

	// 로딩스크린 DataTable 로딩 완료 감지
	void OnConfigTableLoaded();
	
	// 로딩 스크린 에셋 로드 완료 콜백 
	void OnLoadingScreenAssetLoaded();

	// 맵 이름으로 로딩 설정 찾기
	const FSFMapLoadingConfig* FindLoadingConfigForMap(const FString& MapName);

	// GameViewport에서 Widget 제거
	void RemoveWidgetFromViewport();

private:
	bool bCurrentLoadingScreenStarted;

	// 로딩 설정 DataTable 
	UPROPERTY(Config, EditDefaultsOnly, Category = "SF|Loading")
	TSoftObjectPtr<UDataTable> LoadingConfigTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CachedConfigTable;

	TSharedPtr<FStreamableHandle> ConfigTableLoadHandle;
	TSharedPtr<FStreamableHandle> CurrentWidgetLoadHandle;

	// Preload 요청된 레벨 이름 
	FString PendingLevelName;
    
	// Preload 완료된 위젯 클래스
	UPROPERTY(Transient)
	TSubclassOf<UUserWidget> PreloadedWidgetClass;

	// Hard Travel 로딩 스크린 위젯 클래스 (Host 로딩 스크린 위젯)
	UPROPERTY(Config, EditDefaultsOnly, Category = "SF|Loading")
	FSoftClassPath HardLoadingScreenWidget;
	
	TSharedPtr<SWidget> HardLoadingSWidgetPtr;

	UPROPERTY(BlueprintAssignable, meta=(AllowPrivateAccess))
	FLoadingScreenWidgetChangedDelegate OnHardLoadingScreenWidgetChanged;
	
	UPROPERTY(BlueprintAssignable, meta=(AllowPrivateAccess))
	FLoadingScreenWidgetChangedDelegate OnSeamlessLoadingScreenWidgetChanged;

	// 현재 설정된 HardLoadingScreen 위젯 클래스 (Host 로딩 스크린 위젯의 자식 위젯으로 사실상 로딩 화면 표시)
	UPROPERTY()
	TSubclassOf<UUserWidget> HardLoadingScreenWidgetClass;

	// 현재 설정된 SeamlessLoadingScreen 위젯 클래스 
	UPROPERTY()
	TSubclassOf<UUserWidget> SeamlessLoadingScreenWidgetClass;
	
};
