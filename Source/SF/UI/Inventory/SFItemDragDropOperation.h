#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Item/SFItemManagerComponent.h"
#include "SFItemDragDropOperation.generated.h"

class USFItemEntryWidget;
class USFItemInstance;
class USFQuickbarComponent;
class USFInventoryManagerComponent;
/**
 * 
 */
UCLASS()
class SF_API USFItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	USFItemDragDropOperation(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 드래그 시작 슬롯 */
	UPROPERTY()
	FSFItemSlotHandle FromSlot;

	/** 드래그 중인 아이템 인스턴스 */
	UPROPERTY()
	TObjectPtr<USFItemInstance> DraggedItemInstance;

	/** 드래그 시작한 엔트리 위젯 (투명도 조절용) */
	UPROPERTY()
	TObjectPtr<USFItemEntryWidget> FromEntryWidget;
};
