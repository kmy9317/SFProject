// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "SFAssetManager.generated.h"

class USFPawnData;
class USFHeroDefinition;

/**
 * 
 */
UCLASS()
class SF_API USFAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	static USFAssetManager& Get();
	void LoadCharacterDefinitions(const FStreamableDelegate& LoadFinishedCallback);
	bool GetLoadedCharacterDefinitions(TArray<USFHeroDefinition*>& LoadedHeroDefinations) const;

	TSharedPtr<FStreamableHandle> LoadPawnDataAsync(const TSoftObjectPtr<USFPawnData>& PawnDataPath, FStreamableDelegate& LoadCompleteDelegate);
	
};
