#include "SFCreateRoomWidget.h"

#include "System/SFOSSGameInstance.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/SpinBox.h"
#include "Components/CheckBox.h"
#include "Kismet/GameplayStatics.h"

void USFCreateRoomWidget::NativeConstruct()
{
    Super::NativeConstruct();

    //===========================GameInstance 캐스팅==========================
    GameInstance = Cast<USFOSSGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    //=======================================================================

    //==============================이벤트 바인딩==============================
    if (CreateButton)
    {
        CreateButton->OnClicked.AddDynamic(this, &USFCreateRoomWidget::OnCreateButtonClicked);
    }

    if (CancelButton)
    {
        CancelButton->OnClicked.AddDynamic(this, &USFCreateRoomWidget::OnCancelButtonClicked);
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
        MaxPlayersSpinBox->SetValue(1.0f);
        MaxPlayersSpinBox->SetMinValue(1.0f);
        MaxPlayersSpinBox->SetMaxValue(4.0f);
    }
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
