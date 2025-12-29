#pragma once

#include "CoreMinimal.h"
#include "SFWidgetController.h"
#include "SFSharedOverlayWidgetController.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class SF_API USFSharedOverlayWidgetController : public USFWidgetController
{
	GENERATED_BODY()

public:
	// TODO : 향후 SharedOverlay 전용 데이터 바인딩 가능
};
