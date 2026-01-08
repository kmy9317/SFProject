#pragma once

#include "CoreMinimal.h"
#include "UI/Inventory/SFItemEntryWidget.h"
#include "SFQuickbarEntryWidget.generated.h"

class USFQuickbarSlotWidget;

/**
 * 
 */
UCLASS()
class SF_API USFQuickbarEntryWidget : public USFItemEntryWidget
{
	GENERATED_BODY()

public:
	USFQuickbarEntryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void Init(USFQuickbarSlotWidget* InSlotWidget, USFItemInstance* InItemInstance, int32 InItemCount);

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

private:
	UPROPERTY()
	TObjectPtr<USFQuickbarSlotWidget> OwningSlotWidget;
};
