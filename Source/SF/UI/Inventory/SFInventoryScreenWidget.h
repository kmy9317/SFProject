#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFInventoryScreenWidget.generated.h"

class USFItemDropWidget;
class USFInventorySlotsWidget;
class USFQuickbarSlotsWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnScreenClosed);

/**
 * 
 */
UCLASS()
class SF_API USFInventoryScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USFInventoryScreenWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "UI")
	void InitializeScreen();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void CloseScreen();

public:
	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnScreenClosed OnScreenClosed;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FKey CloseKey = EKeys::Tab;

	// ItemDrop만 BindWidget으로 직접 참조
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USFItemDropWidget> Widget_ItemDrop;
};
