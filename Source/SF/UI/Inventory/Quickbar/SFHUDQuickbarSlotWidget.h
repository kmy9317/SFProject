#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFHUDQuickbarSlotWidget.generated.h"

class USFItemInstance;
class USFQuickbarComponent;
class UImage;
class UTextBlock;

UCLASS()
class SF_API USFHUDQuickbarSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USFHUDQuickbarSlotWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void OnQuickbarEntryChanged(int32 SlotIndex, USFItemInstance* ItemInstance, int32 ItemCount);

public:
	/** 슬롯 인덱스 (0, 1, 2, 3) */
	UPROPERTY(EditAnywhere, Category = "UI")
	int32 SlotIndex = 0;

	/** 표시할 단축키 번호 (1, 2, 3, 4 등) */
	UPROPERTY(EditAnywhere, Category = "UI")
	int32 DisplayNumber = 1;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SlotNumber;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Count;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Highlight;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> Animation_Highlight_In;

private:
	UPROPERTY()
	TObjectPtr<USFQuickbarComponent> QuickbarComponent;

	FDelegateHandle EntryChangedDelegateHandle;
};
