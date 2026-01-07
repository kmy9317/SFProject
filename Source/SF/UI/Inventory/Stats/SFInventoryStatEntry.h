#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "SFInventoryStatEntry.generated.h"

class USFStatHoverWidget;
/**
 * 
 */
UCLASS()
class SF_API USFInventoryStatEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	USFInventoryStatEntry(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeDestruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

protected:
	// 블루프린트에서 설정할 스탯 태그 (예: Stat.AttackPower)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat", meta = (Categories = "Stat"))
	FGameplayTag StatTag;

private:
	void ShowHoverWidget(const FPointerEvent& InMouseEvent);
	void HideHoverWidget();

private:
	UPROPERTY()
	TObjectPtr<USFStatHoverWidget> HoverWidget;
};
