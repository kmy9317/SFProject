#include "SFQuickbarSlotsWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "UI/SFUIData.h"
#include "UI/Inventory/Quickbar/SFQuickbarSlotWidget.h"
#include "Inventory/SFQuickbarComponent.h"
#include "Item/SFItemInstance.h"

USFQuickbarSlotsWidget::USFQuickbarSlotsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USFQuickbarSlotsWidget::NativeDestruct()
{
	DestructUI();

	Super::NativeDestruct();
}

void USFQuickbarSlotsWidget::InitializeWithQuickbar(USFQuickbarComponent* InQuickbarComponent)
{
	if (InQuickbarComponent)
	{
		ConstructUI(InQuickbarComponent);
	}
}

void USFQuickbarSlotsWidget::ConstructUI(USFQuickbarComponent* InQuickbarComponent)
{
	if (InQuickbarComponent == nullptr)
	{
		return;
	}

	DestructUI();

	QuickbarComponent = InQuickbarComponent;

	TSubclassOf<USFQuickbarSlotWidget> SlotWidgetClass = USFUIData::Get().QuickbarSlotWidgetClass;
	if (!SlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("QuickbarSlotWidgetClass is not set in UIData!"));
		return;
	}

	const int32 SlotCount = QuickbarComponent->GetSlotCount();
	SlotWidgets.SetNum(SlotCount);

	for (int32 i = 0; i < SlotCount; ++i)
	{
		USFQuickbarSlotWidget* SlotWidget = CreateWidget<USFQuickbarSlotWidget>(GetOwningPlayer(), SlotWidgetClass);
		SlotWidget->Init(i, QuickbarComponent);
		SlotWidgets[i] = SlotWidget;

		UHorizontalBoxSlot* BoxSlot = HorizontalBox_Slots->AddChildToHorizontalBox(SlotWidget);
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
		BoxSlot->SetVerticalAlignment(VAlign_Center);
		BoxSlot->SetPadding(FMargin(SlotPadding));
	}

	// 기존 아이템 로드
	const TArray<FSFQuickbarEntry>& Entries = QuickbarComponent->GetAllEntries();
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		const FSFQuickbarEntry& Entry = Entries[i];
		if (USFItemInstance* ItemInst = Entry.GetItemInstance())
		{
			OnQuickbarEntryChanged(i, ItemInst, Entry.GetItemCount());
		}
	}

	// 델리게이트 바인딩
	EntryChangedDelegateHandle = QuickbarComponent->OnQuickbarEntryChanged.AddUObject(
		this,
		&ThisClass::OnQuickbarEntryChanged
	);
}

void USFQuickbarSlotsWidget::DestructUI()
{
	if (QuickbarComponent && EntryChangedDelegateHandle.IsValid())
	{
		QuickbarComponent->OnQuickbarEntryChanged.Remove(EntryChangedDelegateHandle);
		EntryChangedDelegateHandle.Reset();
	}

	HorizontalBox_Slots->ClearChildren();
	SlotWidgets.Empty();

	QuickbarComponent = nullptr;
}

void USFQuickbarSlotsWidget::OnQuickbarEntryChanged(int32 SlotIndex, USFItemInstance* ItemInstance, int32 ItemCount)
{
	if (SlotWidgets.IsValidIndex(SlotIndex))
	{
		SlotWidgets[SlotIndex]->OnQuickbarEntryChanged(ItemInstance, ItemCount);
	}
}