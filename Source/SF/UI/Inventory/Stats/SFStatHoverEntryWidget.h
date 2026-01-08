#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFStatHoverEntryWidget.generated.h"

class URichTextBlock;
class UTextBlock;
/**
 * 
 */
UCLASS()
class SF_API USFStatHoverEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USFStatHoverEntryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void RefreshUI(const FText& Name, const FText& Description);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URichTextBlock> Text_Description;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> Animation_FadeIn;
};
