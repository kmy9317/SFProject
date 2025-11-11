// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "SFHeroEntryWidget.generated.h"

class UImage;
class USFHeroDefinition;
class UTextBlock;

/**
 * 
 */
UCLASS()
class SF_API USFHeroEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	const USFHeroDefinition* GetHeroDefinition() const { return HeroDefinition; }
	void SetSelected(bool bIsSelected);
	
private:
	UPROPERTY(meta =(BindWidget))
	TObjectPtr<UImage> HeroIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HeroNameText;

	UPROPERTY(EditDefaultsOnly, Category = "SF|MaterialParams")
	FName IconTextureMatParamName = "Icon";

	UPROPERTY(EditDefaultsOnly, Category = "SF|MaterialParams")
	FName SaturationMatParamName = "Saturation";

	UPROPERTY()
	TObjectPtr<USFHeroDefinition> HeroDefinition;
};
