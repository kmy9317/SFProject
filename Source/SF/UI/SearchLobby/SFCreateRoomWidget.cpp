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

void USFCreateRoomWidget::NativeConstruct()
{
    Super::NativeConstruct();

    bIsFocusable = true;
    
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

    if (SecretRoomCheckBox)
    {
        SecretRoomCheckBox->OnCheckStateChanged.AddDynamic(this, &USFCreateRoomWidget::OnSecretRoomCheckboxChanged);

        bool bChecked = SecretRoomCheckBox->IsChecked();
        if (PasswordInput)
        {
            PasswordInput->SetIsEnabled(bChecked);
        }
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

    if (MaxPlayersSpinBox)
    {
        MaxPlayersSpinBox->SetValue(1);
        MaxPlayersSpinBox->SetMinValue(1);
        MaxPlayersSpinBox->SetMaxValue(4);
    }

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

    FString RoomName = RoomNameInput->GetText().ToString();
    FString Password = PasswordInput ? PasswordInput->GetText().ToString() : TEXT("");
    int32 MaxPlayers = static_cast<int32>(MaxPlayersSpinBox ? MaxPlayersSpinBox->GetValue() : 4.0f);
    bool bProtected = !Password.IsEmpty();

    if (RoomName.IsEmpty())
    {
        if (ErrorMessageText)
        {
            ErrorMessageText->SetText(FText::FromString(TEXT("방 이름을 입력해주세요")));
        }
        return;
    }

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

void USFCreateRoomWidget::OnCreateSessionComplete(bool bWasSuccessful, const FString& Message)
{
    if (bWasSuccessful)
    {
        UE_LOG(LogTemp, Warning, TEXT("Session created success: %s"), *Message);

        if (GameInstance)
        {
            GameInstance->LoadWaitingLevel_AsHost();
        }

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
