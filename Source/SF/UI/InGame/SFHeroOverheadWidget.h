#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFHeroOverheadWidget.generated.h"

class UTextBlock;
class UProgressBar;

UCLASS()
class SF_API USFHeroOverheadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 순수 표시 함수들 (로직 없음)
	UFUNCTION(BlueprintCallable, Category = "SF|UI")
	void SetPlayerName(const FString& Name);

	UFUNCTION(BlueprintCallable, Category = "SF|UI")
	void SetReviveGaugeVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "SF|UI")
	void SetReviveGaugePercent(float Percent);

protected:
	// UPROPERTY(meta = (BindWidget))
	// TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> ReviveGaugePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ReviveProgressBar;
};
