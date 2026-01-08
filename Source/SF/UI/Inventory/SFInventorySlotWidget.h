#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFInventorySlotWidget.generated.h"

class UImage;
class UOverlay;
class USizeBox;
class USFItemInstance;
class USFInventoryEntryWidget;
class USFInventoryManagerComponent;

/**
 * 인벤토리 개별 슬롯
 */
UCLASS()
class SF_API USFInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USFInventorySlotWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void Init(int32 InSlotIndex, USFInventoryManagerComponent* InInventoryManager);

protected:
	virtual void NativeOnInitialized() override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

public:
	void OnInventoryEntryChanged(USFItemInstance* InItemInstance, int32 InItemCount);

	int32 GetSlotIndex() const { return SlotIndex; }
	USFInventoryManagerComponent* GetInventoryManager() const { return InventoryManager; }

private:
	void SetValidationVisual(bool bIsValid);
	void ClearValidationVisual();

	/** 이동 가능 여부 검증 (클라이언트 예측용) */
	bool CanAcceptDrop(class USFItemDragDropOperation* DragDrop) const;

protected:
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<USFInventoryManagerComponent> InventoryManager;

	UPROPERTY()
	TObjectPtr<USFInventoryEntryWidget> EntryWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USFInventoryEntryWidget> EntryWidgetClass;

	//~ BindWidget
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox_Root;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Background;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_Entry;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Valid;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Invalid;
};
