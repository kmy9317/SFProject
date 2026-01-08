#include "SFHUDQuickbarSlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Inventory/SFQuickbarComponent.h"
#include "Item/SFItemInstance.h"
#include "Item/SFItemDefinition.h"
#include "Item/SFItemData.h"

USFHUDQuickbarSlotWidget::USFHUDQuickbarSlotWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USFHUDQuickbarSlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 슬롯 번호 표시
	if (Text_SlotNumber)
	{
		Text_SlotNumber->SetText(FText::AsNumber(DisplayNumber));
	}
}

void USFHUDQuickbarSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 초기 상태: 아이콘, 수량 숨김
	if (Image_Icon)
	{
		Image_Icon->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Text_Count)
	{
		Text_Count->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Image_Highlight)
	{
		Image_Highlight->SetVisibility(ESlateVisibility::Hidden);
	}

	// QuickbarComponent 찾기
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	QuickbarComponent = PC->FindComponentByClass<USFQuickbarComponent>();
	if (!QuickbarComponent)
	{
		return;
	}

	// 기존 아이템 로드
	if (QuickbarComponent->IsValidSlotIndex(SlotIndex))
	{
		USFItemInstance* ItemInstance = QuickbarComponent->GetItemInstance(SlotIndex);
		int32 ItemCount = QuickbarComponent->GetItemCount(SlotIndex);
		OnQuickbarEntryChanged(SlotIndex, ItemInstance, ItemCount);
	}

	// 델리게이트 바인딩
	EntryChangedDelegateHandle = QuickbarComponent->OnQuickbarEntryChanged.AddUObject(this, &ThisClass::OnQuickbarEntryChanged);
}

void USFHUDQuickbarSlotWidget::NativeDestruct()
{
	if (QuickbarComponent && EntryChangedDelegateHandle.IsValid())
	{
		QuickbarComponent->OnQuickbarEntryChanged.Remove(EntryChangedDelegateHandle);
		EntryChangedDelegateHandle.Reset();
	}

	Super::NativeDestruct();
}

void USFHUDQuickbarSlotWidget::OnQuickbarEntryChanged(int32 InSlotIndex, USFItemInstance* ItemInstance, int32 ItemCount)
{
	// 내 슬롯이 아니면 무시
	if (InSlotIndex != SlotIndex)
	{
		return;
	}

	if (ItemInstance && ItemCount > 0)
	{
		// 아이콘 표시
		const USFItemDefinition* ItemDef = USFItemData::Get().FindDefinitionById(ItemInstance->GetItemID());
		if (ItemDef && ItemDef->Icon)
		{
			Image_Icon->SetBrushFromTexture(ItemDef->Icon, true);
			Image_Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}

		// 수량 표시 (2개 이상일 때만)
		if (ItemCount > 1)
		{
			Text_Count->SetText(FText::AsNumber(ItemCount));
			Text_Count->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Text_Count->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else
	{
		// 아이템 없음
		Image_Icon->SetVisibility(ESlateVisibility::Hidden);
		Text_Count->SetVisibility(ESlateVisibility::Hidden);
	}
}