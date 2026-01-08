#include "SFInventorySlotsWidget.h"

#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "UI/Inventory/SFInventorySlotWidget.h"
#include "Inventory/SFInventoryManagerComponent.h"
#include "Item/SFItemInstance.h"
#include "UI/SFUIData.h"

USFInventorySlotsWidget::USFInventorySlotsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USFInventorySlotsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Text_Title)
	{
		Text_Title->SetText(TitleText);
	}
}

void USFInventorySlotsWidget::NativeDestruct()
{
	DestructUI();

	Super::NativeDestruct();
}

void USFInventorySlotsWidget::InitializeWithInventory(USFInventoryManagerComponent* InInventoryManager)
{
	if (InInventoryManager)
	{
		ConstructUI(InInventoryManager);
	}
}

void USFInventorySlotsWidget::ConstructUI(USFInventoryManagerComponent* InInventoryManager)
{
	if (InInventoryManager == nullptr)
	{
		return;
	}
	// 기존 UI 정리
	DestructUI();

	InventoryManager = InInventoryManager;

	GridPanel_Slots->SetSlotPadding(FMargin(SlotPadding));

	TSubclassOf<USFInventorySlotWidget> SlotWidgetClass = USFUIData::Get().InventorySlotWidgetClass;
	if (!SlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventorySlotWidgetClass is not set in UIData!"));
		return;
	}

	const int32 SlotCount = InventoryManager->GetSlotCount();
	SlotWidgets.SetNum(SlotCount);

	for (int32 i = 0; i < SlotCount; ++i)
	{
		USFInventorySlotWidget* SlotWidget = CreateWidget<USFInventorySlotWidget>(GetOwningPlayer(), SlotWidgetClass);
		SlotWidget->Init(i, InventoryManager);
		SlotWidgets[i] = SlotWidget;

		const int32 Row = i / GridColumns;
		const int32 Column = i % GridColumns;
		GridPanel_Slots->AddChildToUniformGrid(SlotWidget, Row, Column);
	}

	// 기존 아이템 로드
	const TArray<FSFInventoryEntry>& Entries = InventoryManager->GetAllEntries();
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		const FSFInventoryEntry& Entry = Entries[i];
		if (USFItemInstance* ItemInst = Entry.GetItemInstance())
		{
			OnInventoryEntryChanged(i, ItemInst, Entry.GetItemCount());
		}
	}

	// 델리게이트 바인딩
	EntryChangedDelegateHandle = InventoryManager->OnInventoryEntryChanged.AddUObject(
		this,
		&ThisClass::OnInventoryEntryChanged
	);
}

void USFInventorySlotsWidget::DestructUI()
{
	// 델리게이트 해제
	if (InventoryManager && EntryChangedDelegateHandle.IsValid())
	{
		InventoryManager->OnInventoryEntryChanged.Remove(EntryChangedDelegateHandle);
		EntryChangedDelegateHandle.Reset();
	}

	// 슬롯 위젯 정리
	GridPanel_Slots->ClearChildren();
	SlotWidgets.Empty();

	InventoryManager = nullptr;
}

void USFInventorySlotsWidget::OnInventoryEntryChanged(int32 SlotIndex, USFItemInstance* ItemInstance, int32 ItemCount)
{
	if (SlotWidgets.IsValidIndex(SlotIndex))
	{
		SlotWidgets[SlotIndex]->OnInventoryEntryChanged(ItemInstance, ItemCount);
	}
}