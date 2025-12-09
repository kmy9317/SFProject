#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFPasswordInputWidget.generated.h"

class USFOSSGameInstance;
class UEditableTextBox;
class UTextBlock;
class UCommonButtonBase;

UCLASS()
class SF_API USFPasswordInputWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void SetSessionIndex(int32 InSessionIndex); //입장 시도할 세션 인덱스 설정

protected:
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

public:
	//====================================UI 위젯====================================
	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* PasswordInputBox; //비밀번호 입력란

	UPROPERTY(meta = (BindWidget))
	UCommonButtonBase* ConfirmButton; //확인 버튼

	UPROPERTY(meta = (BindWidget))
	UCommonButtonBase* CancelButton; //취소 버튼

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ErrorMessageText; //오류 메시지 표시(안쓸 가능성 높음)
	//================================================================================

	//===================================콜백 함수=====================================
	UFUNCTION()
	void OnConfirmButtonClicked(); //비밀번호 확인 클릭

	UFUNCTION()
	void OnCancelButtonClicked(); //취소 클릭

	UFUNCTION()
	void OnJoinSessionComplete(bool bWasSuccessful, const FString& Message); //세션 입장 완료 콜백
	//================================================================================

private:
	int32 SessionIndex; //입장할 세션 인덱스

	UPROPERTY()
	USFOSSGameInstance* GameInstance; //세션 입장 처리를 수행하는 GameInstance
};