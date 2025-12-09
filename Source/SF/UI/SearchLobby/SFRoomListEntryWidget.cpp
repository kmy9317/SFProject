#include "SFRoomListEntryWidget.h"
#include "SFSessionListItem.h"
#include "System/SFOSSGameInstance.h"
#include "SFPasswordInputWidget.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/GameplayStatics.h"

#include "UI/Common/CommonButtonBase.h"

void USFRoomListEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    //============================GameInstance 캐스팅============================
    GameInstance = Cast<USFOSSGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    //==========================================================================
    
    //===============================이벤트 바인딩================================
    if (JoinButton)
    {
        JoinButton->OnButtonClickedDelegate.AddDynamic(this, &USFRoomListEntryWidget::OnJoinButtonClicked);
    }

    if (GameInstance)
    {
        GameInstance->OnJoinSessionComplete_Sig.AddUObject(this, &USFRoomListEntryWidget::OnJoinSessionComplete);
    }
    //==========================================================================
}

//==============ListView에서 해당 Row에 데이터가 설정될 때 호출=================
void USFRoomListEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    const USFSessionListItem* Item = Cast<USFSessionListItem>(ListItemObject);
    if (!Item) return;
    
    SessionIndex = Item->SessionIndex;

    if (SessionIndex >= -1)
    {
        if (MainSwitcher)
        {
            // 가짜 방 목록 (빈방 이미지) 출력 후 데이터 세팅 없이 종료
            MainSwitcher->SetActiveWidgetIndex(1);
            return;
        }
    }

    if (MainSwitcher)
    {
        MainSwitcher->SetActiveWidgetIndex(0);
    }

    SessionInfo = Item->Data;
    
    if (RoomNameText) RoomNameText->SetText(FText::FromString(SessionInfo.RoomName));
    if (PlayerCountText) PlayerCountText->SetText(FText::FromString(FString::Printf(TEXT("👥 %d/%d"), SessionInfo.CurrentPlayers, SessionInfo.MaxPlayers)));
    if (HostNameText) HostNameText->SetText(FText::FromString(SessionInfo.HostName));
    if (ProtectedIndicator) ProtectedIndicator->SetText(SessionInfo.bIsPasswordProtected ? FText::FromString(TEXT("🔒")) : FText::FromString(TEXT("—")));
}   
//========================================================================

//==============================이벤트 & 콜백===============================
void USFRoomListEntryWidget::OnJoinButtonClicked()
{
    if (!GameInstance) return;

    if (SessionInfo.bIsPasswordProtected)
    {
        ShowPasswordDialog(); //비밀방일 경우 비밀번호 입력창 띄우기
    }
    else
    {
        GameInstance->JoinGameSession(SessionIndex, TEXT("")); //일반 방 입장
    }
}

//세션 입장 콜백
void USFRoomListEntryWidget::OnJoinSessionComplete(bool bWasSuccessful, const FString& Message)
{
    //입장 실패 메시지를 표시하거나 UI 연동 가능 (안쓰는 중)
}
//==========================================================================

//==============================비밀번호 입력창 표시===========================
void USFRoomListEntryWidget::ShowPasswordDialog()
{
    if (!GetOwningPlayer() || !PasswordInputWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[RoomListEntryWidget] PasswordInputWidgetClass is NULL or OwningPlayer is NULL"));
        return;
    }

    USFPasswordInputWidget* PW = CreateWidget<USFPasswordInputWidget>(GetOwningPlayer(), PasswordInputWidgetClass);
    if (PW)
    {
        PW->SetSessionIndex(SessionIndex);
        PW->AddToViewport(100);
    }
}
//==========================================================================
