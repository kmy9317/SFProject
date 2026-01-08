#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFItemEntryWidget.generated.h"

class UTextBlock;
class UImage;
class USFItemInstance;
/**
 * 
 */
UCLASS()
class SF_API USFItemEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USFItemEntryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

public:
	/** UI 갱신 */
	void RefreshUI(USFItemInstance* InItemInstance, int32 InItemCount);
	void RefreshItemCount(int32 InItemCount);
	
	/** 드래그 시 투명도 조절 */
	void SetDragOpacity(bool bIsDragging);

	USFItemInstance* GetItemInstance() const { return ItemInstance; }
	int32 GetItemCount() const { return ItemCount; }

protected:
	UPROPERTY()
	TObjectPtr<USFItemInstance> ItemInstance;

	int32 ItemCount = 0;

	//~ BindWidget
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_RarityBG;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Hover;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Count;
};
