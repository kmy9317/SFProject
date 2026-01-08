#include "SFInventoryStatEntry.h"
#include "UI/SFUIData.h"
#include "UI/Inventory/Stats/SFStatHoverWidget.h"

USFInventoryStatEntry::USFInventoryStatEntry(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USFInventoryStatEntry::NativeDestruct()
{
	HideHoverWidget();
	Super::NativeDestruct();
}

void USFInventoryStatEntry::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	ShowHoverWidget(InMouseEvent);
}

FReply USFInventoryStatEntry::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply Reply = Super::NativeOnMouseMove(InGeometry, InMouseEvent);

	if (HoverWidget)
	{
		HoverWidget->SetPosition(InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled();
	}

	return Reply;
}

void USFInventoryStatEntry::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	HideHoverWidget();
}

void USFInventoryStatEntry::ShowHoverWidget(const FPointerEvent& InMouseEvent)
{
	if (!StatTag.IsValid())
	{
		return;
	}

	const FSFUIInfo& UIInfo = USFUIData::Get().GetTagUIInfo(StatTag);
	if (!UIInfo.IsValid())
	{
		return;
	}

	if (!HoverWidget)
	{
		TSubclassOf<USFStatHoverWidget> HoverWidgetClass = USFUIData::Get().StatHoverWidgetClass;
		if (HoverWidgetClass)
		{
			HoverWidget = CreateWidget<USFStatHoverWidget>(GetOwningPlayer(), HoverWidgetClass);
		}
	}

	if (HoverWidget)
	{
		HoverWidget->RefreshUI(UIInfo.Title, UIInfo.Description);
		HoverWidget->AddToViewport(100);
		HoverWidget->SetPosition(InMouseEvent.GetScreenSpacePosition());
	}
}

void USFInventoryStatEntry::HideHoverWidget()
{
	if (HoverWidget)
	{
		HoverWidget->RemoveFromParent();
		HoverWidget = nullptr;
	}
}