#include "SFItemDragWidget.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/SFUIData.h"

USFItemDragWidget::USFItemDragWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USFItemDragWidget::Init(UTexture2D* InIcon, int32 InItemCount)
{
	const FIntPoint& SlotSize = USFUIData::Get().SlotSize;
	SizeBox_Root->SetWidthOverride(SlotSize.X);
	SizeBox_Root->SetHeightOverride(SlotSize.Y);

	Image_Icon->SetBrushFromTexture(InIcon, true);
	Text_Count->SetText(InItemCount > 1 ? FText::AsNumber(InItemCount) : FText::GetEmpty());
}