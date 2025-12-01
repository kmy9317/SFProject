#include "SFSimpleLoadingScreenWidget.h"

#define LOCTEXT_NAMESPACE "UMG"

void USFSimpleLoadingScreenWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	LoadingSlate->SetRotatingBrush(&RotatingBrush);
	LoadingSlate->SetBackgroundBrush(&BackgroundBrush);
	LoadingSlate->SetRotationSpeed(RotationSpeed);
}

void USFSimpleLoadingScreenWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	LoadingSlate.Reset();
}

TSharedRef<SWidget> USFSimpleLoadingScreenWidget::RebuildWidget()
{
	LoadingSlate = SNew(SSFSimpleLoadingScreen)
	.RotatingBrush(&RotatingBrush)
	.BackgroundBrush(&BackgroundBrush)
	.RotationSpeed(RotationSpeed);
	return LoadingSlate.ToSharedRef();
}

#if WITH_EDITOR
const FText USFSimpleLoadingScreenWidget::GetPaletteCategory()
{
	return LOCTEXT("CustomPaletteCategory", "SF Slate");
}
#endif