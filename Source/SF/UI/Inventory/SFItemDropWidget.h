#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFItemDropWidget.generated.h"

/**
 * 버리기 영역
 */
UCLASS()
class SF_API USFItemDropWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USFItemDropWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
};
