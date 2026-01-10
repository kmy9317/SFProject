#include "UI/MainMenu/SFCreditWidget.h"

#include "Components/ScrollBox.h"
#include "Input/Reply.h"
#include "TimerManager.h"
#include "UI/Common/CommonButtonBase.h"

void USFCreditWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bEndSequenceStarted = false;

	if (IsValid(Button_Back))
	{
		Button_Back->OnButtonClickedDelegate.AddDynamic(this, &USFCreditWidget::CloseWidget);
	}

	// 시작 시 스크롤 초기화 및 데이터 로드
	if (CreditScrollBox)
	{
		CreditScrollBox->SetScrollOffset(0.0f);
		CurrentScrollOffset = 0.0f;
	}

	// 2. 데이터 테이블 읽어서 화면 채우기
	PopulateCredits();
}

void USFCreditWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!CreditScrollBox) return;

	CurrentScrollOffset += (ScrollSpeed * InDeltaTime);
	CreditScrollBox->SetScrollOffset(CurrentScrollOffset);

	if (CurrentScrollOffset >= CreditScrollBox->GetScrollOffsetOfEnd())
	{
		// 아직 종료 예약 안 걸었으면 (최초 1회만 실행)
		if (!bEndSequenceStarted)
		{
			bEndSequenceStarted = true;

			if (UWorld* World = GetWorld())
			{
				FTimerHandle TimerHandle;
				World->GetTimerManager().SetTimer(TimerHandle, this, &USFCreditWidget::CloseWidget, EndDelay, false);
			}
		}
	}
}

FReply USFCreditWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		CloseWidget();
		return FReply::Handled();
	}
	
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void USFCreditWidget::PopulateCredits()
{
	if (!CreditDataTable) return;

	if (CreditScrollBox)
	{
		CreditScrollBox->ClearChildren();
	}

	FString ContextString;
	TArray<FSFCreditRow*> AllRows;
	CreditDataTable->GetAllRows<FSFCreditRow>(ContextString, AllRows);

	FString LastSectionTitle = "";

	for (FSFCreditRow* Row : AllRows)
	{
		if (!Row) continue;

		// 1. 섹션 제목이 이전과 다르면 -> BP에서 헤더 생성
		if (!Row->SectionTitle.Equals(LastSectionTitle))
		{
			BP_AddSectionHeader(Row->SectionTitle);
			LastSectionTitle = Row->SectionTitle;
		}

		// 2. 내용 줄 -> BP에서 내용 생성
		// 구조체 값(*Row)을 그대로 넘겨줌
		BP_AddCreditRow(*Row);
	}
}

void USFCreditWidget::CloseWidget()
{
	RemoveFromParent();
}
