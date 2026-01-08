#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFQuickbarSlotWidget.generated.h"

class UImage;
class UOverlay;
class USizeBox;
class USFItemInstance;
class USFQuickbarEntryWidget;
class USFQuickbarComponent;
class USFItemDragDropOperation;

/**
 * 
 */
UCLASS()
class SF_API USFQuickbarSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USFQuickbarSlotWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void Init(int32 InSlotIndex, USFQuickbarComponent* InQuickbarComponent);

protected:
	virtual void NativeOnInitialized() override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

public:
	void OnQuickbarEntryChanged(USFItemInstance* InItemInstance, int32 InItemCount);

	int32 GetSlotIndex() const { return SlotIndex; }
	USFQuickbarComponent* GetQuickbarComponent() const { return QuickbarComponent; }

private:
	void SetValidationVisual(bool bIsValid);
	void ClearValidationVisual();
	bool CanAcceptDrop(USFItemDragDropOperation* DragDrop) const;

protected:
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<USFQuickbarComponent> QuickbarComponent;

	UPROPERTY()
	TObjectPtr<USFQuickbarEntryWidget> EntryWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox_Root;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Background;

	/** 빈 슬롯일 때 표시할 기본 아이콘 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_BaseIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_Entry;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Valid;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Invalid;
};
