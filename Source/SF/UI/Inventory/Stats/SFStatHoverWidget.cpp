#include "SFStatHoverWidget.h"

#include "SFStatHoverEntryWidget.h"

USFStatHoverWidget::USFStatHoverWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USFStatHoverWidget::RefreshUI(const FText& Name, const FText& Description)
{
	if (Widget_HoverEntry)
	{
		Widget_HoverEntry->RefreshUI(Name, Description);
	}
}