#include "SFUserWidget.h"

void USFUserWidget::SetWidgetController(USFWidgetController* InWidgetController)
{
	WidgetController = InWidgetController;
	OnWidgetControllerSet();
}
