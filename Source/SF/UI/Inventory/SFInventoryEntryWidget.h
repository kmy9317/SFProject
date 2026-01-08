#pragma once

#include "CoreMinimal.h"
#include "SFItemEntryWidget.h"
#include "SFInventoryEntryWidget.generated.h"

class USFItemDragWidget;
class USFInventorySlotWidget;
/**
 * 인벤토리 아이템 엔트리
 */
UCLASS()
class SF_API USFInventoryEntryWidget : public USFItemEntryWidget
{
	GENERATED_BODY()

public:
	USFInventoryEntryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void Init(USFInventorySlotWidget* InSlotWidget, USFItemInstance* InItemInstance, int32 InItemCount);

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

private:
	UPROPERTY()
	TObjectPtr<USFInventorySlotWidget> OwningSlotWidget;
};
