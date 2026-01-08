#include "SFItemDropWidget.h"

#include "SFItemDragDropOperation.h"
#include "SFItemEntryWidget.h"
#include "Item/SFItemManagerComponent.h"

USFItemDropWidget::USFItemDropWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool USFItemDropWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

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

	// ItemManager를 통해 서버에 드롭(버리기) 요청
	USFItemManagerComponent* ItemManager = GetOwningPlayer()->FindComponentByClass<USFItemManagerComponent>();
	if (ItemManager == nullptr)
	{
		return false;
	}

	ItemManager->Server_DropItem(DragDrop->FromSlot);

	return true;
}