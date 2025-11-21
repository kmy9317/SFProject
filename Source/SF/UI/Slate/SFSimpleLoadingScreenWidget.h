#pragma once

#include "CoreMinimal.h"
#include "SFSimpleLoadingScreen.h"
#include "Components/Widget.h"
#include "SFSimpleLoadingScreenWidget.generated.h"

/**
 * 
 */
UCLASS()
class SF_API USFSimpleLoadingScreenWidget : public UWidget
{
	GENERATED_BODY()

public:
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance")
	FSlateBrush RotatingBrush;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance")
	FSlateBrush BackgroundBrush;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rotation")
	float RotationSpeed = 0.2f;

	
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	TSharedPtr<SSFSimpleLoadingScreen> LoadingSlate;
};
