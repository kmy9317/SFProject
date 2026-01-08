#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFQuickbarSlotsWidget.generated.h"

class UHorizontalBox;
class USFQuickbarSlotWidget;
class USFQuickbarComponent;
class USFItemInstance;

UCLASS()
class SF_API USFQuickbarSlotsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USFQuickbarSlotsWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "UI")
	void InitializeWithQuickbar(USFQuickbarComponent* InQuickbarComponent);

protected:
	virtual void NativeDestruct() override;

private:
	void ConstructUI(USFQuickbarComponent* InQuickbarComponent);
	void DestructUI();
	void OnQuickbarEntryChanged(int32 SlotIndex, USFItemInstance* ItemInstance, int32 ItemCount);

protected:
	UPROPERTY()
	TObjectPtr<USFQuickbarComponent> QuickbarComponent;

	UPROPERTY()
	TArray<TObjectPtr<USFQuickbarSlotWidget>> SlotWidgets;

	FDelegateHandle EntryChangedDelegateHandle;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> HorizontalBox_Slots;

	UPROPERTY(EditAnywhere, Category = "SF|UI")
	float SlotPadding = 4.f;
};
