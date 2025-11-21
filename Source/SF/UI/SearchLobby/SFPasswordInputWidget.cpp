#include "SFPasswordInputWidget.h"

#include "System/SFOSSGameInstance.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void USFPasswordInputWidget::NativeConstruct()
{
    Super::NativeConstruct();

    //=============================GameInstance 캐스팅==============================
    GameInstance = Cast<USFOSSGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    //=============================================================================

    //================================이벤트 바인딩=================================
    if (ConfirmButton)
    {
        ConfirmButton->OnClicked.AddDynamic(this, &USFPasswordInputWidget::OnConfirmButtonClicked);
    }

    if (CancelButton)
    {
        CancelButton->OnClicked.AddDynamic(this, &USFPasswordInputWidget::OnCancelButtonClicked);
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

    FString Password = PasswordInputBox->GetText().ToString();

    if (Password.IsEmpty())
    {
        if (ErrorMessageText)
        {
            ErrorMessageText->SetText(FText::FromString(TEXT("비밀번호 입력")));
        }
        return;
    }

    //비밀번호와 함께 세션 입장 시도
    GameInstance->JoinGameSession(SessionIndex, Password);
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