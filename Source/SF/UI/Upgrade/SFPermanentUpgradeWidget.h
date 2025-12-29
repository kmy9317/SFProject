// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidget.h"
#include "System/SFPlayFabSubsystem.h"
#include "System/Data/SFPermanentUpgradeTypes.h"
#include "SFPermanentUpgradeWidget.generated.h"

class UTextBlock;
class UButton;
class UHorizontalBox;
class USFPermanentUpgradeCardWidget;

UCLASS()
class SF_API USFPermanentUpgradeWidget : public USFUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidget)) UTextBlock* TextSoul = nullptr;
	UPROPERTY(meta=(BindWidget)) UTextBlock* TextCanUpgradeCount = nullptr;

	UPROPERTY(meta=(BindWidget)) UHorizontalBox* BoxCards = nullptr;

	UPROPERTY(meta=(BindWidget)) UButton* ButtonApply = nullptr;
	UPROPERTY(meta=(BindWidget)) UButton* ButtonCancel = nullptr;
	UPROPERTY(meta=(BindWidget)) UButton* ButtonReset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SF|UI")
	TSubclassOf<USFPermanentUpgradeCardWidget> CardWidgetClass;

	// WBP에서 이 DataTable 에셋을 지정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SF|UI")
	UDataTable* UpgradeUIDescTable = nullptr;

private:
	static constexpr int32 MaxCategoryLevel = 30;
	static constexpr int32 MaxTotalLevel = 50;
	static constexpr int32 CostPerLevel = 30;

	UPROPERTY() USFPlayFabSubsystem* Subsystem = nullptr;

	FPlayerStats OriginalStats;
	FPlayerStats WorkingStats;

	UPROPERTY() TArray<USFPermanentUpgradeCardWidget*> CardWidgets;

	FTimerHandle PollLoadedTimerHandle;
	int32 PollTryCount = 0;

	void TryInitFromSubsystem();
	void BuildCards();
	void RefreshAll();

	int32 GetTotalLevel(const FPlayerStats& Stats) const;
	int32 GetPoints(const FPlayerStats& Stats, ESFUpgradeCategory Category) const;
	int32& GetPointsRef(FPlayerStats& Stats, ESFUpgradeCategory Category);

	bool CanIncrease(ESFUpgradeCategory Category) const;
	bool CanDecrease(ESFUpgradeCategory Category) const;

	void Increase(ESFUpgradeCategory Category);
	void Decrease(ESFUpgradeCategory Category);

	bool IsDirty() const;

	// TextBonus: "스탯이름: (설정수치 * 찍은수치)"
	FText MakeBonusText(ESFUpgradeCategory Category, int32 Level) const;

	// Tier desc는 DT에서 읽음
	void GetTierDescs(ESFUpgradeCategory Category, FText& OutT1, FText& OutT2, FText& OutT3) const;

	UFUNCTION() void HandleApply();
	UFUNCTION() void HandleCancel();
	UFUNCTION() void HandleReset();

	void CloseSelf();

	void OnCardPlus(ESFUpgradeCategory Category);
	void OnCardMinus(ESFUpgradeCategory Category);

	// 일괄 조정 기능
	int32 GetModifyStep() const;
	void IncreaseBy(ESFUpgradeCategory Category, int32 Step);
	void DecreaseBy(ESFUpgradeCategory Category, int32 Step);
};
