#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class UCommonBarBase;

UCLASS()
class SF_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonBarBase> HpBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonBarBase> MpBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonBarBase> SpBar;

public:
	UFUNCTION(BlueprintCallable, Category = "UI/InGame")
	void UpdateHp(float InPercent);
	UFUNCTION(BlueprintCallable, Category = "UI/InGame")
	void UpdateMp(float InPercent);
	UFUNCTION(BlueprintCallable, Category = "UI/InGame")
	void UpdateSp(float InPercent);
};
