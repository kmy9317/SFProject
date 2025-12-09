#include "SFPasswordInputWidget.h"

#include "System/SFOSSGameInstance.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

#include "UI/Common/CommonButtonBase.h"

void USFPasswordInputWidget::NativeConstruct()
{
    Super::NativeConstruct();

    //=============================GameInstance 캐스팅==============================
    GameInstance = Cast<USFOSSGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    //=============================================================================

    //================================이벤트 바인딩=================================
    if (ConfirmButton)
    {
        ConfirmButton->OnButtonClickedDelegate.AddDynamic(this, &USFPasswordInputWidget::OnConfirmButtonClicked);
    }

    if (CancelButton)
    {
        CancelButton->OnButtonClickedDelegate.AddDynamic(this, &USFPasswordInputWidget::OnCancelButtonClicked);
    }
    //============================================================================

    //=============================세션 입장 완료 바인딩==============================
    if (GameInstance)
    {
        GameInstance->OnJoinSessionComplete_Sig.AddUObject(this, &USFPasswordInputWidget::OnJoinSessionComplete);
    }
    //============================================================================

    //==================================UI 초기값==================================
    if (PasswordInputBox)
    {
        PasswordInputBox->SetText(FText::FromString(TEXT("비밀번호를 입력하세요")));
    }
    //============================================================================
}

//==============================버튼 이벤트==============================
void USFPasswordInputWidget::OnConfirmButtonClicked()
{
    if (!GameInstance || !PasswordInputBox)
    {
        return;
    }

    FString InputPassword = PasswordInputBox->GetText().ToString();

    if (InputPassword.IsEmpty())
    {
        if (ErrorMessageText)
        {
            ErrorMessageText->SetText(FText::FromString(TEXT("비밀번호 입력")));
        }
        return;
    }
    
    //실제 세션 비밀번호 읽기
    const TArray<FSessionInfo>& Sessions = GameInstance->GetAvailableSessions();
    if (!Sessions.IsValidIndex(SessionIndex))
    {
        if (ErrorMessageText)
        {
            ErrorMessageText->SetText(FText::FromString(TEXT("세션 정보 오류")));
        }
        return;
    }

    FString RealPassword;
    Sessions[SessionIndex].SearchResult.Session.SessionSettings.Get(
        TEXT("PASSWORD"),
        RealPassword
    );
    
    //비밀번호 검증
    if (!RealPassword.Equals(InputPassword))
    {
        if (ErrorMessageText)
        {
            ErrorMessageText->SetText(FText::FromString(TEXT("비밀번호가 일치하지 않습니다")));
        }
        return;
    }
    
    //성공할 경우 JoinSession
    GameInstance->JoinGameSession(SessionIndex, InputPassword);
}

void USFPasswordInputWidget::OnCancelButtonClicked()
{
    RemoveFromParent(); //위젯 닫기
}
//========================================================================

//============================세션 입장 완료 콜백============================
void USFPasswordInputWidget::OnJoinSessionComplete(bool bWasSuccessful, const FString& Message)
{
    if (bWasSuccessful)
    {
        RemoveFromParent(); //입장 성공 시 위젯 제거
    }
    else
    {
        if (ErrorMessageText)
        {
            ErrorMessageText->SetText(FText::FromString(Message)); //실패 메시지 표시
        }
    }
}
//=========================================================================

//==============================세션 인덱스 설정==============================
void USFPasswordInputWidget::SetSessionIndex(int32 InSessionIndex)
{
    SessionIndex = InSessionIndex;
}
//=========================================================================

//================================인풋 이벤트(ESC 창 닫기 기능)================================
FReply USFPasswordInputWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (CancelButton)
        {
            CancelButton->OnButtonClicked();
        }
        
        return FReply::Handled();
    }
    
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
//=========================================================================