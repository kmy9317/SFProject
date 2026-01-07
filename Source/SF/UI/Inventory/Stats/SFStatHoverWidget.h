#pragma once

#include "CoreMinimal.h"
#include "UI/Common/SFHoverWidget.h"
#include "SFStatHoverWidget.generated.h"

class USFStatHoverEntryWidget;
class URichTextBlock;
class UTextBlock;
/**
 * 
 */
UCLASS()
class SF_API USFStatHoverWidget : public USFHoverWidget
{
	GENERATED_BODY()

public:
	USFStatHoverWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void RefreshUI(const FText& Name, const FText& Description);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USFStatHoverEntryWidget> Widget_HoverEntry;
};
