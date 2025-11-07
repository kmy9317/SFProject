// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFTeamSelectionWidget.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSlotClicked, uint8 /*SlotID*/);

class UTextBlock;
class UButton;

/**
 * 
 */
UCLASS()
class SF_API USFTeamSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	
	void SetSlotID(uint8 NewSlotID);
	void UpdateSlotInfo(const FString& PlayerNickname);
	
private:
	UFUNCTION()
	void SelectButtonClicked();

public:
	FOnSlotClicked OnSlotClicked;
	
private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> SelectButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> InfoText;

	uint8 SlotID;
};
