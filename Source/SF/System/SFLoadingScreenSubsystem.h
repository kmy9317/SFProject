// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SFLoadingScreenSubsystem.generated.h"

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

	// 수동으로 로딩 스크린 시작 
	UFUNCTION(BlueprintCallable, Category = "SF|Loading")
	void StartLoadingScreen();
	
	// 수동으로 로딩 스크린 종료
	UFUNCTION(BlueprintCallable, Category = "SF|Loading")
	void StopLoadingScreen();

private:
	// 하드 트래블 감지용 (OpenLevel)
	void OnPreLoadMap(const FString& MapName);

	// 심리스 트래블 감지용 (ServerTravel)
	void OnSeamlessTravelStart(UWorld* CurrentWorld, const FString& LevelName);

	// 맵 로딩 완료 감지 (트랜지션 맵과 목적지 맵 구분 로직 포함)
	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);

	bool IsInSeamlessTravel();
	
	// 트랜지션 맵인지 확인하는 헬퍼 함수
	bool IsTransitionMap(const FString& MapName);

	void RemoveWidgetFromViewport();

private:
	bool bIsSeamlessTravelInProgress;

	bool bCurrentLoadingScreenStarted;
	
	UPROPERTY(Config, EditDefaultsOnly, Category = "SF|Loading")
	FSoftClassPath LoadingScreenWidget;;

	TSharedPtr<SWidget> LoadingSWidgetPtr;
};
