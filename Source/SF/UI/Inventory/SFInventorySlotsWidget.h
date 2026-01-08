#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "SFInventorySlotsWidget.generated.h"

class UTextBlock;
class UUniformGridPanel;
class USFInventorySlotWidget;
class USFInventoryManagerComponent;
class USFItemInstance;

/**
 * 인벤토리 전체 관리
 */
UCLASS()
class SF_API USFInventorySlotsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USFInventorySlotsWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 외부에서 직접 초기화 호출 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void InitializeWithInventory(USFInventoryManagerComponent* InInventoryManager);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void ConstructUI(USFInventoryManagerComponent* InInventoryManager);
	void DestructUI();
	void OnInventoryEntryChanged(int32 SlotIndex, USFItemInstance* ItemInstance, int32 ItemCount);

public:
	UPROPERTY(EditAnywhere)
	FText TitleText = FText::FromString(TEXT("Inventory"));

	UPROPERTY(EditAnywhere)
	int32 GridColumns = 5;

	UPROPERTY(EditAnywhere, Category = "SF|UI")
	float SlotPadding = 4.f;  // 

protected:
	UPROPERTY()
	TObjectPtr<USFInventoryManagerComponent> InventoryManager;

	UPROPERTY()
	TArray<TObjectPtr<USFInventorySlotWidget>> SlotWidgets;

	FDelegateHandle EntryChangedDelegateHandle;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Title;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> GridPanel_Slots;
};
