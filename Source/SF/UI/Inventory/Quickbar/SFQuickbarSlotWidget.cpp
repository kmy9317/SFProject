#include "SFQuickbarSlotWidget.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "UI/SFUIData.h"
#include "UI/Inventory/Quickbar/SFQuickbarEntryWidget.h"
#include "UI/Inventory/SFItemDragDropOperation.h"
#include "UI/Inventory/SFItemEntryWidget.h"
#include "Item/SFItemInstance.h"
#include "Item/SFItemManagerComponent.h"
#include "Item/SFItemDefinition.h"
#include "Item/SFItemData.h"
#include "Inventory/SFQuickbarComponent.h"

USFQuickbarSlotWidget::USFQuickbarSlotWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USFQuickbarSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	const FIntPoint& SlotSize = USFUIData::Get().SlotSize;
	SizeBox_Root->SetWidthOverride(SlotSize.X);
	SizeBox_Root->SetHeightOverride(SlotSize.Y);
}

void USFQuickbarSlotWidget::Init(int32 InSlotIndex, USFQuickbarComponent* InQuickbarComponent)
{
	check(InSlotIndex >= 0 && InQuickbarComponent != nullptr);

	SlotIndex = InSlotIndex;
	QuickbarComponent = InQuickbarComponent;
}

bool USFQuickbarSlotWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);

	USFItemDragDropOperation* DragDrop = Cast<USFItemDragDropOperation>(InOperation);
	if (DragDrop == nullptr || DragDrop->DraggedItemInstance == nullptr)
	{
		return false;
	}

	// 같은 슬롯이면 무시
	if (DragDrop->FromSlot.SlotType == ESFItemSlotType::Quickbar && DragDrop->FromSlot.SlotIndex == SlotIndex)
	{
		ClearValidationVisual();
		return true;
	}

	bool bCanDrop = CanAcceptDrop(DragDrop);
	SetValidationVisual(bCanDrop);

	return true;
}

void USFQuickbarSlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);

	ClearValidationVisual();
}

bool USFQuickbarSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

	ClearValidationVisual();

	USFItemDragDropOperation* DragDrop = Cast<USFItemDragDropOperation>(InOperation);
	if (DragDrop == nullptr)
	{
		return false;
	}

	// 드래그 시작 위젯 투명도 복원
	if (USFItemEntryWidget* FromEntry = DragDrop->FromEntryWidget)
	{
		FromEntry->SetDragOpacity(false);
	}

	// 같은 슬롯이면 무시
	if (DragDrop->FromSlot.SlotType == ESFItemSlotType::Quickbar && DragDrop->FromSlot.SlotIndex == SlotIndex)
	{
		return true;
	}

	// ItemManager를 통해 서버에 이동 요청
	USFItemManagerComponent* ItemManager = GetOwningPlayer()->FindComponentByClass<USFItemManagerComponent>();
	if (ItemManager == nullptr)
	{
		return false;
	}

	FSFItemSlotHandle ToSlot(ESFItemSlotType::Quickbar, SlotIndex);
	ItemManager->Server_MoveItem(DragDrop->FromSlot, ToSlot);

	return true;
}

void USFQuickbarSlotWidget::OnQuickbarEntryChanged(USFItemInstance* InItemInstance, int32 InItemCount)
{
	// 기존 엔트리 제거
	if (EntryWidget)
	{
		Overlay_Entry->RemoveChild(EntryWidget);
		EntryWidget = nullptr;
	}

	// 새 아이템이 있으면 엔트리 생성
	if (InItemInstance && InItemCount > 0)
	{
		TSubclassOf<USFQuickbarEntryWidget> WidgetClass = USFUIData::Get().QuickbarEntryWidgetClass;
		if (WidgetClass)
		{
			EntryWidget = CreateWidget<USFQuickbarEntryWidget>(GetOwningPlayer(), WidgetClass);
			if (EntryWidget)
			{
				EntryWidget->Init(this, InItemInstance, InItemCount);

				UOverlaySlot* OverlaySlot = Overlay_Entry->AddChildToOverlay(EntryWidget);
				OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
				OverlaySlot->SetVerticalAlignment(VAlign_Fill);
			}
		}

		// 아이템 있으면 기본 아이콘 숨김
		Image_BaseIcon->SetRenderOpacity(0.f);
	}
	else
	{
		// 아이템 없으면 기본 아이콘 표시
		Image_BaseIcon->SetRenderOpacity(1.f);
	}
}

bool USFQuickbarSlotWidget::CanAcceptDrop(USFItemDragDropOperation* DragDrop) const
{
	if (!DragDrop || !DragDrop->DraggedItemInstance)
	{
		return false;
	}

	return true;
}

void USFQuickbarSlotWidget::SetValidationVisual(bool bIsValid)
{
	if (bIsValid)
	{
		Image_Valid->SetVisibility(ESlateVisibility::HitTestInvisible);
		Image_Invalid->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		Image_Valid->SetVisibility(ESlateVisibility::Hidden);
		Image_Invalid->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void USFQuickbarSlotWidget::ClearValidationVisual()
{
	Image_Valid->SetVisibility(ESlateVisibility::Hidden);
	Image_Invalid->SetVisibility(ESlateVisibility::Hidden);
}