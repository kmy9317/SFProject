// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidget.h"
#include "System/Data/SFPermanentUpgradeTypes.h"
#include "SFPermanentUpgradeCardWidget.generated.h"

class UTextBlock;
class UButton;
class UBorder;
class UImage;

UCLASS()
class SF_API USFPermanentUpgradeCardWidget : public USFUserWidget
{
	GENERATED_BODY()

public:
	// 카드 초기 설정(카테고리 + 티어 설명 3줄)
	void Setup(
		ESFUpgradeCategory InCategory,
		const FText& InTier1Desc,
		const FText& InTier2Desc,
		const FText& InTier3Desc
	);

	// 매 프레임/매 클릭마다 UI 갱신
	void Refresh(
		int32 CurrentLevel,
		int32 MaxLevel,
		bool bPlusEnabled,
		bool bMinusEnabled,
		const FText& BonusText
	);

	ESFUpgradeCategory GetCategory() const { return Category; }

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlusClicked, ESFUpgradeCategory);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnMinusClicked, ESFUpgradeCategory);

	FOnPlusClicked OnPlusClicked;
	FOnMinusClicked OnMinusClicked;

protected:
	virtual void NativeConstruct() override;

	// ===== BindWidget (WBP에서 변수명 동일하게 만들기) =====
	UPROPERTY(meta=(BindWidget)) UTextBlock* TextTitle = nullptr;

	UPROPERTY(meta=(BindWidget)) UButton* ButtonMinus = nullptr;
	UPROPERTY(meta=(BindWidget)) UTextBlock* TextLevel = nullptr;
	UPROPERTY(meta=(BindWidget)) UButton* ButtonPlus = nullptr;

	UPROPERTY(meta=(BindWidget)) UTextBlock* TextBonus = nullptr;

	// Tier 라인(활성/비활성 느낌용)
	UPROPERTY(meta=(BindWidget)) UBorder* BorderTier1 = nullptr;
	UPROPERTY(meta=(BindWidget)) UBorder* BorderTier2 = nullptr;
	UPROPERTY(meta=(BindWidget)) UBorder* BorderTier3 = nullptr;

	UPROPERTY(meta=(BindWidget)) UTextBlock* TextTier1 = nullptr;
	UPROPERTY(meta=(BindWidget)) UTextBlock* TextTier2 = nullptr;
	UPROPERTY(meta=(BindWidget)) UTextBlock* TextTier3 = nullptr;

	// 선택: 체크 아이콘(없어도 동작)
	UPROPERTY(meta=(BindWidgetOptional)) UImage* ImageTier1Check = nullptr;
	UPROPERTY(meta=(BindWidgetOptional)) UImage* ImageTier2Check = nullptr;
	UPROPERTY(meta=(BindWidgetOptional)) UImage* ImageTier3Check = nullptr;

private:
	ESFUpgradeCategory Category = ESFUpgradeCategory::Wrath;

	UFUNCTION() void HandlePlus();
	UFUNCTION() void HandleMinus();

	FText GetCategoryLabel() const;

	void SetTierVisual(int32 CurrentLevel);
	void SetTierLine(UBorder* Border, UImage* CheckImage, bool bActive);
};

