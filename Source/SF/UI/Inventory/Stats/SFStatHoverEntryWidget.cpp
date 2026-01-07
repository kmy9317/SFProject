#include "SFStatHoverEntryWidget.h"

#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"

USFStatHoverEntryWidget::USFStatHoverEntryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USFStatHoverEntryWidget::RefreshUI(const FText& Name, const FText& Description)
{
	if (Text_Name)
	{
		Text_Name->SetText(Name);
	}

	if (Text_Description)
	{
		Text_Description->SetText(Description);
	}

	if (Animation_FadeIn)
	{
		PlayAnimationForward(Animation_FadeIn);
	}
}