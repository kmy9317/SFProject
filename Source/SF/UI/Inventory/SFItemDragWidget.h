#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFItemDragWidget.generated.h"

class UImage;
class UTextBlock;
class USizeBox;
/**
 * 드래그 비주얼
 */
UCLASS()
class SF_API USFItemDragWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USFItemDragWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void Init(UTexture2D* InIcon, int32 InItemCount);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox_Root;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Count;
};
