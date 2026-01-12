#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interface/SFMiniMapTrackable.h" // Enum 때문에 필요
#include "SFMinimapWidget.generated.h"

class USFMiniMapIcon;
class UCanvasPanel;
class USFMinimapSubsystem;
class UImage;

UCLASS()
class SF_API USFMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void OnTargetRegistered(TScriptInterface<ISFMiniMapTrackable> Target);

	UFUNCTION()
	void OnTargetUnregistered(TScriptInterface<ISFMiniMapTrackable> Target);

protected:
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> MiniMapCanvas;
	
	UPROPERTY(EditDefaultsOnly, Category = "MiniMap")
	TSubclassOf<USFMiniMapIcon> IconWidgetClass;
	
	UPROPERTY(EditAnywhere, Category = "MiniMap")
	float MiniMapScale = 1.0f;
	
	UPROPERTY(EditAnywhere, Category = "MiniMap")
	float MapRadius = 125.0f;

private:
	UPROPERTY()
	TMap<UObject*, USFMiniMapIcon*> TargetIcons;

	UPROPERTY()
	USFMinimapSubsystem* MiniMapSubsystem;

	void UpdateIconPositions();
};