// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SFOSSGameInstance.h"
#include "Engine/GameInstance.h"
#include "SFGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFGameInstance : public USFOSSGameInstance
{
	GENERATED_BODY()

public:
	void StartMatch();
	void LoadLevelAndListen(TSoftObjectPtr<UWorld> Level);
	
protected:
	virtual void Init() override;

private:
	
	/*
	* TODO : 추후 다른 방식으로 Map 참조 및 로드 할 수 있음
	*/
	UPROPERTY(EditDefaultsOnly, Category = "SF|Map")
	TSoftObjectPtr<UWorld> MainMenuLevel;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Map")
	TSoftObjectPtr<UWorld> LobbyLevel;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Map")
	TSoftObjectPtr<UWorld> GameLevel;
};
