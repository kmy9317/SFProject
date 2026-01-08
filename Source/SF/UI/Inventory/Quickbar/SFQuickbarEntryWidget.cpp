#include "SFQuickbarEntryWidget.h"
#include "UI/Inventory/Quickbar/SFQuickbarSlotWidget.h"
#include "UI/Inventory/SFItemDragDropOperation.h"
#include "UI/Inventory/SFItemDragWidget.h"
#include "UI/SFUIData.h"
#include "Item/SFItemInstance.h"
#include "Item/SFItemDefinition.h"
#include "Item/SFItemData.h"
#include "Item/SFItemManagerComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

USFQuickbarEntryWidget::USFQuickbarEntryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USFQuickbarEntryWidget::Init(USFQuickbarSlotWidget* InSlotWidget, USFItemInstance* InItemInstance, int32 InItemCount)
{
	if (InSlotWidget == nullptr || InItemInstance == nullptr)
	{
		return;
	}

	OwningSlotWidget = InSlotWidget;
	RefreshUI(InItemInstance, InItemCount);
}

FReply USFQuickbarEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply Reply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	// 우클릭: 아이템 사용
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && !UWidgetBlueprintLibrary::IsDragDropping())
	{
		if (OwningSlotWidget)
		{
			USFItemManagerComponent* ItemManager = GetOwningPlayer()->FindComponentByClass<USFItemManagerComponent>();
			if (ItemManager)
			{
				FSFItemSlotHandle ItemSlot(ESFItemSlotType::Quickbar, OwningSlotWidget->GetSlotIndex());
				ItemManager->Server_UseItem(ItemSlot);
				return FReply::Handled();
			}
		}
	}

	return Reply;
}

void USFQuickbarEntryWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (OwningSlotWidget == nullptr || ItemInstance == nullptr)
	{
		return;
	}

	// 드래그 비주얼 위젯 생성
	USFItemDragWidget* DragWidget = nullptr;
	TSubclassOf<USFItemDragWidget> DragWidgetClass = USFUIData::Get().ItemDragWidgetClass;
	if (DragWidgetClass)
	{
		DragWidget = CreateWidget<USFItemDragWidget>(GetOwningPlayer(), DragWidgetClass);
		if (DragWidget)
		{
			const USFItemDefinition* ItemDef = USFItemData::Get().FindDefinitionById(ItemInstance->GetItemID());
			if (ItemDef)
			{
				DragWidget->Init(ItemDef->Icon, ItemCount);
			}
		}
	}

	// 드래그 드롭 오퍼레이션 생성
	USFItemDragDropOperation* DragDrop = NewObject<USFItemDragDropOperation>();
	DragDrop->DefaultDragVisual = DragWidget;
	DragDrop->Pivot = EDragPivot::CenterCenter;
	DragDrop->FromSlot = FSFItemSlotHandle(ESFItemSlotType::Quickbar, OwningSlotWidget->GetSlotIndex());
	DragDrop->DraggedItemInstance = ItemInstance;
	DragDrop->FromEntryWidget = this;

	OutOperation = DragDrop;
}