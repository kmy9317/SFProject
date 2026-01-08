#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SFUIData.generated.h"

class USFQuickbarEntryWidget;
class USFQuickbarSlotWidget;
class USFInventoryEntryWidget;
class USFInventorySlotWidget;
class USFItemDragWidget;
class USFStatHoverWidget;

UENUM(BlueprintType)
enum class ESFStatDisplayType : uint8
{
	Integer,      // 100 → "100"
	Decimal,      // 1.25 → "1.3"
	Percent,      // 0.15 → "15%"
	PerSecond     // 0.05 (per 0.1s tick) → "0.5/s"
};

USTRUCT(BlueprintType)
struct FSFUIInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText Title;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ESFStatDisplayType DisplayType = ESFStatDisplayType::Integer;

	bool IsValid() const { return !Title.IsEmpty(); }
};
/**
 * 
 */
UCLASS()
class SF_API USFUIData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const USFUIData& Get();

	UFUNCTION(BlueprintPure, Category = "UI")
	static FText FormatStatValue(float Value, ESFStatDisplayType DisplayType);

	UFUNCTION(BlueprintPure, Category = "UI", meta = (DisplayName = "Get Tag UI Info"))
	static const FSFUIInfo& GetTagUIInfoStatic(const FGameplayTag& Tag);

	UFUNCTION(BlueprintPure, Category = "UI")
	const FSFUIInfo& GetTagUIInfo(const FGameplayTag& Tag) const;

	//~ 슬롯 크기
	UPROPERTY(EditDefaultsOnly, Category = "Slot")
	FIntPoint SlotSize = FIntPoint(80, 80);

	//~ 인벤토리 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<USFInventorySlotWidget> InventorySlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<USFInventoryEntryWidget> InventoryEntryWidgetClass;

	//~ 퀵바 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Quickbar")
	TSubclassOf<USFQuickbarSlotWidget> QuickbarSlotWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Quickbar")
	TSubclassOf<USFQuickbarEntryWidget> QuickbarEntryWidgetClass;

	//~ 공통 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Common")
	TSubclassOf<USFItemDragWidget> ItemDragWidgetClass;

	//~ 스탯 호버 위젯
	UPROPERTY(EditDefaultsOnly, Category = "Stat")
	TSubclassOf<USFStatHoverWidget> StatHoverWidgetClass;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Common", meta = (DisplayName = "Tag UI Infos", Categories = "Stat"))
	TMap<FGameplayTag, FSFUIInfo> TagUIInfos;
};
