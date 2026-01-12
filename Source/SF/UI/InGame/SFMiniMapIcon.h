// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidget.h"
#include "SFMiniMapIcon.generated.h"

enum class EMiniMapIconType : uint8;
class UImage;
/**
 * 
 */
UCLASS()
class SF_API USFMiniMapIcon : public USFUserWidget
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, Category = "MiniMap")
	void SetTarget(TScriptInterface<ISFMiniMapTrackable> InTarget);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;
	
	UPROPERTY(EditDefaultsOnly, Category = "MiniMap")
	TMap<EMiniMapIconType, UTexture2D*> IconTextures;
	
	UPROPERTY(EditDefaultsOnly, Category = "MiniMap")
	float BossSizeMultiplier = 1.5f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MiniMap")
	FLinearColor PlayerColor = FLinearColor::Green;

	UPROPERTY(EditDefaultsOnly, Category = "MiniMap")
	FLinearColor AllyColor = FLinearColor::Blue;

	UPROPERTY(EditDefaultsOnly, Category = "MiniMap")
	FLinearColor EnemyColor = FLinearColor::Red;

	UPROPERTY(EditDefaultsOnly, Category = "MiniMap")
	FLinearColor BossColor = FLinearColor(1.0f, 0.5f, 0.0f); 

private:
	UPROPERTY()
	TScriptInterface<ISFMiniMapTrackable> TrackedTarget;
    
	float BlinkTimer = 0.f;
	bool bBlinkState = false;
};
