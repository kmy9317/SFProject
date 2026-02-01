#include "SFCreateRoomWidget.h"

#include "System/SFOSSGameInstance.h"
#include "UI/Common/CommonButtonBase.h"

#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/SpinBox.h"
#include "Components/CheckBox.h"
#include "Kismet/GameplayStatics.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Components/Button.h"

void USFCreateRoomWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    //===========================GameInstance 캐스팅==========================
    GameInstance = Cast<USFOSSGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    //=======================================================================

    //==============================이벤트 바인딩==============================
    if (CreateButton)
    {
        CreateButton->OnButtonClickedDelegate.AddDynamic(this, &USFCreateRoomWidget::OnCreateButtonClicked);
    }

    if (CancelButton)
    {
        CancelButton->OnButtonClickedDelegate.AddDynamic(this, &USFCreateRoomWidget::OnCancelButtonClicked);
    }

    if (GameInstance)
    {
        GameInstance->OnCreateSessionComplete_Sig.AddUObject(this, &USFCreateRoomWidget::OnCreateSessionComplete);
    }

    // 비밀방 체크박스 연동
    if (SecretRoomCheckBox)
    {
        SecretRoomCheckBox->OnCheckStateChanged.AddDynamic(this, &USFCreateRoomWidget::OnSecretRoomCheckboxChanged);

        bool bChecked = SecretRoomCheckBox->IsChecked();
        if (PasswordInput)
        {
            PasswordInput->SetIsEnabled(bChecked);
        }
    }
    if (DecreasePlayerCountButton)
    {
        DecreasePlayerCountButton->OnClicked.AddDynamic(this, &USFCreateRoomWidget::OnDecreasePlayerCountClicked);
    }
    if (IncreasePlayerCountButton)
    {
        IncreasePlayerCountButton->OnClicked.AddDynamic(this, &USFCreateRoomWidget::OnIncreasePlayerCountClicked);
    }
    //=======================================================================

    //================================UI 초기값===============================
    if (RoomNameInput)
    {
        RoomNameInput->SetText(FText::FromString(TEXT("")));
    }

    if (PasswordInput)
    {
        PasswordInput->SetText(FText::FromString(TEXT("")));
    }

    // 초기값 설정 (기본 1명 OR 4명)
    CurrentMaxPlayerCount = 1; 
    UpdateMaxPlayerDisplay();

    SetKeyboardFocus();
    //=======================================================================
}

//================================버튼 이벤트================================
void USFCreateRoomWidget::OnCreateButtonClicked()
{
    if (!GameInstance || !RoomNameInput)
    {
        return;
    }

    // 입력값 수집
    FString RoomName = RoomNameInput->GetText().ToString();
    FString Password = PasswordInput ? PasswordInput->GetText().ToString() : TEXT("");
    int32 MaxPlayers = CurrentMaxPlayerCount;
    bool bProtected = !Password.IsEmpty();

    // 유효성 검사
    if (RoomName.IsEmpty())
    {
        if (ErrorMessageText)
        {
            ErrorMessageText->SetText(FText::FromString(TEXT("방 이름을 입력해주세요")));
        }
        return;
    }

    // 비밀번호 저장 (접속시 PreLogin에서 검증)
    if (GameInstance)
    {
        GameInstance->SetSessionPassword(Password);
    }

    GameInstance->CreateGameSession(RoomName, bProtected, MaxPlayers);
}

void USFCreateRoomWidget::OnCancelButtonClicked()
{
    RemoveFromParent();
}

void USFCreateRoomWidget::OnDecreasePlayerCountClicked()
{
    // 최소 1명까지만 감소
    if (CurrentMaxPlayerCount > 1)
    {
        CurrentMaxPlayerCount--;
        UpdateMaxPlayerDisplay();
    }
}

void USFCreateRoomWidget::OnIncreasePlayerCountClicked()
{
    // 최대 4명까지만 증가
    if (CurrentMaxPlayerCount < 4)
    {
        CurrentMaxPlayerCount++;
        UpdateMaxPlayerDisplay();
    }
}

void USFCreateRoomWidget::UpdateMaxPlayerDisplay()
{
    if (MaxPlayersText)
    {
        MaxPlayersText->SetText(FText::AsNumber(CurrentMaxPlayerCount));
    }
}

void USFCreateRoomWidget::OnCreateSessionComplete(bool bWasSuccessful, const FString& Message)
{
    if (bWasSuccessful)
    {
        UE_LOG(LogTemp, Warning, TEXT("Session created success: %s"), *Message);
        RemoveFromParent();
    }
    else
    {
        if (ErrorMessageText)
        {
            ErrorMessageText->SetText(FText::FromString(Message));
        }

        UE_LOG(LogTemp, Error, TEXT("Session creation failed: %s"), *Message);
    }
}

void USFCreateRoomWidget::OnSecretRoomCheckboxChanged(bool bIsChecked)
{
    if (PasswordInput)
    {
        PasswordInput->SetIsEnabled(bIsChecked);
        PasswordInput->SetText(FText::FromString(TEXT("")));
    }
}
//===================================================================================

//================================인풋 이벤트(ESC 창 닫기 기능)================================
FReply USFCreateRoomWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
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
