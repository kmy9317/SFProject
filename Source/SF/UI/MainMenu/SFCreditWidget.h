// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFCreditStruct.h"
#include "SFCreditWidget.generated.h"

class UScrollBox;
class UCommonButtonBase;

/**
 * 크레딧 화면을 제어하는 메인 위젯 클래스
 */
UCLASS()
class SF_API USFCreditWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	// 자동 스크롤을 위해서 호출
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	// 키보드 입력 (ESC)을 위해서 호출
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// 테이터 테이블 읽는 함수
	void PopulateCredits();
	
public:
	// 스크롤 속도 (설정)
	UPROPERTY(EditAnywhere, Category = "UI|Credits")
	float ScrollSpeed = 60.0f;
	
	// 스크롤 다 내려간 뒤 닫힐때까지 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Credits")
	float EndDelay = 3.0f;

protected:
	// [바인딩] UMG에 배치할 스크롤 박스
	UPROPERTY(meta = (BindWidget), BlueprintReadWrite, VisibleAnywhere)
	UScrollBox* CreditScrollBox;

	// 크레딧 데이터 테이블
	UPROPERTY(EditAnywhere, Category = "UI|Credits")
	UDataTable* CreditDataTable;

	// [바인딩] 뒤로가기 버튼
	UPROPERTY(meta = (BindWidget))
	UCommonButtonBase* Button_Back;

	UFUNCTION(BlueprintImplementableEvent, Category="UI|Credits")
	void BP_AddSectionHeader(const FString& Title);

	UFUNCTION(BlueprintImplementableEvent, Category="UI|Credits")
	void BP_AddCreditRow(const FSFCreditRow& RowData);

private:
	// 중복 실행 방지용 플래그
	bool bEndSequenceStarted = false;
	
	float CurrentScrollOffset = 0.0f;

	// 위젯 닫는 함수
	UFUNCTION(BlueprintCallable, Category = "UI|Credits")
	void CloseWidget();
};
