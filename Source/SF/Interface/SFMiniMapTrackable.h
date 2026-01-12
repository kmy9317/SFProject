// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SFMiniMapTrackable.generated.h"


UENUM(BlueprintType)
enum class EMiniMapIconType : uint8
{
	Player      UMETA(DisplayName = "Player"),
	Enemy       UMETA(DisplayName = "Enemy"),
	Boss        UMETA(DisplayName = "Boss")
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USFMiniMapTrackable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SF_API ISFMiniMapTrackable
{
	GENERATED_BODY()
	
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// 월드 좌표 반환 
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MiniMap")
	FVector GetMiniMapWorldPosition() const;
	virtual FVector GetMiniMapWorldPosition_Implementation() const = 0;

	// 아이콘 타입 반환 
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MiniMap")
	EMiniMapIconType GetMiniMapIconType() const;
	virtual EMiniMapIconType GetMiniMapIconType_Implementation() const = 0;

	// 미니맵 표시 여부
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MiniMap")
	bool ShouldShowOnMiniMap() const;
	virtual bool ShouldShowOnMiniMap_Implementation() const { return true; }
};
