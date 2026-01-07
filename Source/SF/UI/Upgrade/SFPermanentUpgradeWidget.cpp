// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Upgrade/SFPermanentUpgradeWidget.h"

#include "UI/Upgrade/SFPermanentUpgradeCardWidget.h"
#include "UI/Common/CommonButtonBase.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "TimerManager.h"
#include "Internationalization/Internationalization.h"
#include "System/Data/SFPermanentUpgradeUIDesc.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

static FName GetRowNameByCategory(ESFUpgradeCategory Cat)
{
	switch (Cat)
	{
	case ESFUpgradeCategory::Wrath: return TEXT("Wrath");
	case ESFUpgradeCategory::Pride: return TEXT("Pride");
	case ESFUpgradeCategory::Lust:  return TEXT("Lust");
	case ESFUpgradeCategory::Sloth: return TEXT("Sloth");
	case ESFUpgradeCategory::Greed: return TEXT("Greed");
	default: return NAME_None;
	}
}

void USFPermanentUpgradeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Apply)  Button_Apply->OnButtonClickedDelegate.AddDynamic(this, &USFPermanentUpgradeWidget::HandleApply);
	if (Button_Cancel) Button_Cancel->OnButtonClickedDelegate.AddDynamic(this, &USFPermanentUpgradeWidget::HandleCancel);
	if (Button_Reset)  Button_Reset->OnClicked.AddDynamic(this, &USFPermanentUpgradeWidget::HandleReset);

	if (UGameInstance* GI = GetGameInstance())
	{
		Subsystem = GI->GetSubsystem<USFPlayFabSubsystem>();
	}

	TryInitFromSubsystem();
}

void USFPermanentUpgradeWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollLoadedTimerHandle);
	}
	Super::NativeDestruct();
}

void USFPermanentUpgradeWidget::TryInitFromSubsystem()
{
	if (!Subsystem) return;

	// 로드가 아직이면 Load 요청 + 폴링
	if (!Subsystem->HasLoadedPlayerData())
	{
		Subsystem->LoadPlayerData();

		if (UWorld* World = GetWorld())
		{
			if (!World->GetTimerManager().IsTimerActive(PollLoadedTimerHandle))
			{
				PollTryCount = 0;
				World->GetTimerManager().SetTimer(
					PollLoadedTimerHandle,
					this,
					&USFPermanentUpgradeWidget::TryInitFromSubsystem,
					0.2f,
					true
				);
			}
		}

		PollTryCount++;
		if (PollTryCount > 200) // 40초쯤
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(PollLoadedTimerHandle);
			}
		}
		return;
	}

	// 로드 완료
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollLoadedTimerHandle);
	}

	OriginalStats = Subsystem->GetPlayerStatsCopy();
	WorkingStats  = OriginalStats;

	BuildCards();
	RefreshAll();
}

void USFPermanentUpgradeWidget::BuildCards()
{
	if (!BoxCards || !CardWidgetClass) return;

	BoxCards->ClearChildren();
	CardWidgets.Empty();

	const ESFUpgradeCategory Categories[5] =
	{
		ESFUpgradeCategory::Wrath,
		ESFUpgradeCategory::Pride,
		ESFUpgradeCategory::Lust,
		ESFUpgradeCategory::Sloth,
		ESFUpgradeCategory::Greed
	};

	for (ESFUpgradeCategory Cat : Categories)
	{
		USFPermanentUpgradeCardWidget* Card = CreateWidget<USFPermanentUpgradeCardWidget>(GetWorld(), CardWidgetClass);
		if (!Card) continue;

		FText T1, T2, T3;
		GetTierDescs(Cat, T1, T2, T3); // ✅ DT에서 읽음
		Card->Setup(Cat, T1, T2, T3);

		Card->OnPlusClicked.AddUObject(this, &USFPermanentUpgradeWidget::OnCardPlus);
		Card->OnMinusClicked.AddUObject(this, &USFPermanentUpgradeWidget::OnCardMinus);

		BoxCards->AddChild(Card);
		CardWidgets.Add(Card);

		// 균등 폭 + 패딩 (Slot 경고 피하려고 변수명 HBoxSlot 사용)
		if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(Card->Slot))
		{
			HBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			HBoxSlot->SetPadding(FMargin(15.0f, 0.0f));
		}
	}
}

void USFPermanentUpgradeWidget::RefreshAll()
{
	// Soul
	if (TextSoul)
	{
		TextSoul->SetText(FText::FromString(FString::Printf(TEXT("%d"), WorkingStats.Gold)));
	}

	// 남은 업그레이드 가능 포인트
	const int32 Total = GetTotalLevel(WorkingStats);
	const int32 Remaining = FMath::Max(0, MaxTotalLevel - Total);

	if (TextCanUpgradeCount)
	{
		TextCanUpgradeCount->SetText(FText::FromString(FString::Printf(TEXT("%d"), Remaining)));
	}

	// Cards
	for (USFPermanentUpgradeCardWidget* Card : CardWidgets)
	{
		if (!Card) continue;

		const ESFUpgradeCategory Cat = Card->GetCategory();
		const int32 Lv = GetPoints(WorkingStats, Cat);

		Card->Refresh(
			Lv,
			MaxCategoryLevel,
			CanIncrease(Cat),
			CanDecrease(Cat),
			MakeBonusText(Cat, Lv)
		);
	}

	// Apply는 변경 있을 때만
	if (Button_Apply)
	{
		Button_Apply->SetIsEnabled(IsDirty());
	}
}

int32 USFPermanentUpgradeWidget::GetTotalLevel(const FPlayerStats& Stats) const
{
	return Stats.Wrath + Stats.Pride + Stats.Lust + Stats.Sloth + Stats.Greed;
}

int32 USFPermanentUpgradeWidget::GetPoints(const FPlayerStats& Stats, ESFUpgradeCategory Category) const
{
	switch (Category)
	{
	case ESFUpgradeCategory::Wrath: return Stats.Wrath;
	case ESFUpgradeCategory::Pride: return Stats.Pride;
	case ESFUpgradeCategory::Lust:  return Stats.Lust;
	case ESFUpgradeCategory::Sloth: return Stats.Sloth;
	case ESFUpgradeCategory::Greed: return Stats.Greed;
	default: return 0;
	}
}

int32& USFPermanentUpgradeWidget::GetPointsRef(FPlayerStats& Stats, ESFUpgradeCategory Category)
{
	switch (Category)
	{
	case ESFUpgradeCategory::Wrath: return Stats.Wrath;
	case ESFUpgradeCategory::Pride: return Stats.Pride;
	case ESFUpgradeCategory::Lust:  return Stats.Lust;
	case ESFUpgradeCategory::Sloth: return Stats.Sloth;
	case ESFUpgradeCategory::Greed: return Stats.Greed;
	default: return Stats.Wrath;
	}
}

bool USFPermanentUpgradeWidget::CanIncrease(ESFUpgradeCategory Category) const
{
	if (WorkingStats.Gold < CostPerLevel) return false;
	if (GetTotalLevel(WorkingStats) >= MaxTotalLevel) return false;

	const int32 Lv = GetPoints(WorkingStats, Category);
	if (Lv >= MaxCategoryLevel) return false;

	return true;
}

bool USFPermanentUpgradeWidget::CanDecrease(ESFUpgradeCategory Category) const
{
	return GetPoints(WorkingStats, Category) > 0;
}

void USFPermanentUpgradeWidget::Increase(ESFUpgradeCategory Category)
{
	if (!CanIncrease(Category)) return;

	int32& Lv = GetPointsRef(WorkingStats, Category);
	Lv += 1;
	WorkingStats.Gold -= CostPerLevel;

	RefreshAll();
}

void USFPermanentUpgradeWidget::Decrease(ESFUpgradeCategory Category)
{
	if (!CanDecrease(Category)) return;

	int32& Lv = GetPointsRef(WorkingStats, Category);
	Lv -= 1;
	WorkingStats.Gold += CostPerLevel;

	RefreshAll();
}

bool USFPermanentUpgradeWidget::IsDirty() const
{
	return OriginalStats.Gold  != WorkingStats.Gold
		|| OriginalStats.Wrath != WorkingStats.Wrath
		|| OriginalStats.Pride != WorkingStats.Pride
		|| OriginalStats.Lust  != WorkingStats.Lust
		|| OriginalStats.Sloth != WorkingStats.Sloth
		|| OriginalStats.Greed != WorkingStats.Greed;
}

FText USFPermanentUpgradeWidget::MakeBonusText(ESFUpgradeCategory Category, int32 Level) const
{
	// DT 없으면 fallback
	if (!UpgradeUIDescTable)
	{
		return FText::FromString(FString::Printf(TEXT("Stat: %d"), Level));
	}

	static const FString Context(TEXT("PermanentUpgradeUIDesc"));
	const FName RowName = GetRowNameByCategory(Category);
	if (RowName.IsNone())
	{
		return FText::GetEmpty();
	}

	const FSFPermanentUpgradeUIDescRow* Row =
		UpgradeUIDescTable->FindRow<FSFPermanentUpgradeUIDescRow>(RowName, Context);

	if (!Row)
	{
		return FText::FromString(TEXT("Stat: ?"));
	}

	// 곱한 결과값
	const float Value = Row->BonusPerLevel * static_cast<float>(Level);

	// 결과값을 보기 좋게(정수면 정수로, 아니면 소수 2자리)
	FNumberFormattingOptions Fmt;
	Fmt.MinimumFractionalDigits = 0;
	Fmt.MaximumFractionalDigits =
		FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value), KINDA_SMALL_NUMBER) ? 0 : 2;

	const FText ValueText = FText::AsNumber(Value, &Fmt);
	
	return FText::Format(
		NSLOCTEXT("SF", "PermanentUpgrade_BonusResultFormat", "{0}: {1}"),
		Row->BonusLabel,
		ValueText
	);
}

void USFPermanentUpgradeWidget::GetTierDescs(ESFUpgradeCategory Category, FText& OutT1, FText& OutT2, FText& OutT3) const
{
	// 기본값(DT 없거나 Row 없을 때)
	OutT1 = FText::FromString(TEXT("T1 description"));
	OutT2 = FText::FromString(TEXT("T2 description"));
	OutT3 = FText::FromString(TEXT("T3 description"));

	if (!UpgradeUIDescTable)
	{
		return;
	}

	static const FString Context(TEXT("PermanentUpgradeUIDesc"));
	const FName RowName = GetRowNameByCategory(Category);
	if (RowName.IsNone())
	{
		return;
	}

	const FSFPermanentUpgradeUIDescRow* Row =
		UpgradeUIDescTable->FindRow<FSFPermanentUpgradeUIDescRow>(RowName, Context);

	if (!Row)
	{
		return;
	}

	OutT1 = Row->Tier1Desc;
	OutT2 = Row->Tier2Desc;
	OutT3 = Row->Tier3Desc;
}

void USFPermanentUpgradeWidget::HandleApply()
{
	if (!Subsystem)
	{
		CloseSelf();
		return;
	}

	Subsystem->SetPlayerStats(WorkingStats);
	Subsystem->SavePlayerData();

	CloseSelf();
}

void USFPermanentUpgradeWidget::HandleCancel()
{
	CloseSelf();
}

void USFPermanentUpgradeWidget::HandleReset()
{
	const int32 Refund = GetTotalLevel(WorkingStats) * CostPerLevel;

	WorkingStats.Wrath = 0;
	WorkingStats.Pride = 0;
	WorkingStats.Lust  = 0;
	WorkingStats.Sloth = 0;
	WorkingStats.Greed = 0;

	WorkingStats.Gold += Refund;

	RefreshAll();
}

void USFPermanentUpgradeWidget::CloseSelf()
{
	RemoveFromParent();
}

void USFPermanentUpgradeWidget::OnCardPlus(ESFUpgradeCategory Category)
{
	IncreaseBy(Category, GetModifyStep());
}

void USFPermanentUpgradeWidget::OnCardMinus(ESFUpgradeCategory Category)
{
	DecreaseBy(Category, GetModifyStep());
}

int32 USFPermanentUpgradeWidget::GetModifyStep() const
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		const bool bShift =
			PC->IsInputKeyDown(EKeys::LeftShift) ||
			PC->IsInputKeyDown(EKeys::RightShift);

		return bShift ? 10 : 1;
	}
	return 1;
}

void USFPermanentUpgradeWidget::IncreaseBy(ESFUpgradeCategory Category, int32 Step)
{
	Step = FMath::Max(1, Step);

	for (int32 i = 0; i < Step; ++i)
	{
		if (!CanIncrease(Category))
		{
			break;
		}

		int32& Lv = GetPointsRef(WorkingStats, Category);
		++Lv;
		WorkingStats.Gold -= CostPerLevel;
	}

	RefreshAll();
}

void USFPermanentUpgradeWidget::DecreaseBy(ESFUpgradeCategory Category, int32 Step)
{
	Step = FMath::Max(1, Step);

	for (int32 i = 0; i < Step; ++i)
	{
		if (!CanDecrease(Category))
		{
			break;
		}

		int32& Lv = GetPointsRef(WorkingStats, Category);
		--Lv;
		WorkingStats.Gold += CostPerLevel;
	}

	RefreshAll();
}
