#include "SFInventoryScreenWidget.h"

#include "SFInventorySlotsWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Inventory/SFInventoryManagerComponent.h"
#include "Inventory/SFQuickbarComponent.h"
#include "Quickbar/SFQuickbarSlotsWidget.h"

USFInventoryScreenWidget::USFInventoryScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void USFInventoryScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetKeyboardFocus();
}

void USFInventoryScreenWidget::NativeDestruct()
{
	UWidgetBlueprintLibrary::CancelDragDrop();

	Super::NativeDestruct();
}

FReply USFInventoryScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	FReply Reply = Super::NativeOnKeyDown(InGeometry, InKeyEvent);

	if (!InKeyEvent.IsRepeat() && InKeyEvent.GetKey() == CloseKey)
	{
		CloseScreen();
		return FReply::Handled();
	}

	return Reply;
}

void USFInventoryScreenWidget::InitializeScreen()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	USFInventoryManagerComponent* InventoryManager = PC->FindComponentByClass<USFInventoryManagerComponent>();
	USFQuickbarComponent* QuickbarComponent = PC->FindComponentByClass<USFQuickbarComponent>();

	TArray<UUserWidget*> FoundWidgets;

	// Inventory 초기화
	if (InventoryManager)
	{
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, FoundWidgets, USFInventorySlotsWidget::StaticClass(), false);
		for (UUserWidget* Widget : FoundWidgets)
		{
			Cast<USFInventorySlotsWidget>(Widget)->InitializeWithInventory(InventoryManager);
		}
	}

	// Quickbar 초기화
	if (QuickbarComponent)
	{
		FoundWidgets.Reset();
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, FoundWidgets, USFQuickbarSlotsWidget::StaticClass(), false);
		for (UUserWidget* Widget : FoundWidgets)
		{
			Cast<USFQuickbarSlotsWidget>(Widget)->InitializeWithQuickbar(QuickbarComponent);
		}
	}
}

void USFInventoryScreenWidget::CloseScreen()
{
	OnScreenClosed.Broadcast();
}