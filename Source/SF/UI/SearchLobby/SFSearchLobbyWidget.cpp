#include "SFSearchLobbyWidget.h"
#include "System/SFOSSGameInstance.h"
#include "SFSessionListItem.h"
#include "SFCreateRoomWidget.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Kismet/GameplayStatics.h"

void USFSearchLobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    //==========================GameInstance 캐스팅==========================
    GameInstance = Cast<USFOSSGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (!GameInstance) return;
    //======================================================================

    //==============================이벤트 바인딩==============================
    if (RefreshButton) RefreshButton->OnClicked.AddDynamic(this, &USFSearchLobbyWidget::OnRefreshButtonClicked);
    if (CreateRoomButton) CreateRoomButton->OnClicked.AddDynamic(this, &USFSearchLobbyWidget::OnCreateRoomButtonClicked);
    if (PasswordProtectedCheckBox) PasswordProtectedCheckBox->OnCheckStateChanged.AddDynamic(this, &USFSearchLobbyWidget::OnPasswordFilterChanged);
    //=======================================================================

    //============================세션 업데이트 바인딩==========================
    GameInstance->OnSessionsUpdated.AddUObject(this, &USFSearchLobbyWidget::OnSessionsUpdated);
    //=======================================================================

    RefreshSessionList(); //초기 세션 업데이트 요청
}

//==============================델리게이트 정리==============================
void USFSearchLobbyWidget::NativeDestruct()
{
    if (GameInstance)
    {
        GameInstance->OnSessionsUpdated.RemoveAll(this); //델리게이트 정리
    }

    Super::NativeDestruct();
}
//=========================================================================

//================================버튼 이벤트================================
void USFSearchLobbyWidget::OnRefreshButtonClicked()
{
    RefreshSessionList(); //새로고침
}

void USFSearchLobbyWidget::OnCreateRoomButtonClicked()
{
    CreateRoomUI(); //방 생성 UI 호출
}

void USFSearchLobbyWidget::OnPasswordFilterChanged(bool bIsChecked)
{
    RefreshSessionList(); //비밀방 표시 여부 변경 시 바로 리스트 업데이트
}
//===========================================================================

//==============================세션 리스트 갱신==============================
void USFSearchLobbyWidget::OnSessionsUpdated(const TArray<FSessionInfo>& Sessions)
{
    if (!SessionListView) return;

    SessionListView->ClearListItems(); //기존 리스트 비우기

    for (int32 i = 0; i < Sessions.Num(); ++i) //Index 포함하여 전달
    {
        SessionListView->AddItem(USFSessionListItem::Make(this, Sessions[i], i)); //리스트 아이템 추가
    }

    if (StatusText)
    {
        StatusText->SetText(
            Sessions.Num() == 0 ?
            FText::FromString(TEXT("검색된 방이 없습니다.")) :
            FText::FromString(FString::Printf(TEXT("검색된 방: %d"), Sessions.Num()))
        );
    }
}
//===========================================================================

//==============================세션 검색 요청=================================
void USFSearchLobbyWidget::RefreshSessionList()
{
    if (GameInstance)
    {
        bool bShowPassword = PasswordProtectedCheckBox && PasswordProtectedCheckBox->IsChecked();
        GameInstance->FindSessions(bShowPassword); //필터 값과 함께 세션 검색 요청
    }
}
//===========================================================================

//====================방 생성 UI(CreateRoomWidget) 띄우기======================
void USFSearchLobbyWidget::CreateRoomUI()
{
    if (!CreateRoomWidgetClass || !GetOwningPlayer()) return;

    USFCreateRoomWidget* Widget = CreateWidget<USFCreateRoomWidget>(this, CreateRoomWidgetClass);
    if (Widget)
    {
        Widget->AddToViewport(10);
    }
}
//===========================================================================
