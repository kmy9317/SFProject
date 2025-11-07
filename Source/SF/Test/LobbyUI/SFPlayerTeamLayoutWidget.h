#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFPlayerTeamLayoutWidget.generated.h"

struct FSFPlayerSelectionInfo;
class UHorizontalBox;
class USFPlayerTeamSlotWidget;

/**
 * 
 */
UCLASS()
class SF_API USFPlayerTeamLayoutWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	void UpdatePlayerSelection(const TArray<FSFPlayerSelectionInfo>& PlayerSelections);
private:	
	UPROPERTY(EditDefaultsOnly, Category = "SF|Visual")
	float PlayerTeamWidgetSlotMargin = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "SF|Visual")
	TSubclassOf<USFPlayerTeamSlotWidget> PlayerTeamSlotWidgetClass;

	// TODO : 팀 구별 없이 Layount 용도로만 사용
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UHorizontalBox> TeamOneLayoutBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UHorizontalBox> TeamTwoLayoutBox;

	UPROPERTY()
	TArray<USFPlayerTeamSlotWidget*> TeamSlotWidgets;
};
