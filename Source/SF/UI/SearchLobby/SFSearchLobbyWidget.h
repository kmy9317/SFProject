#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFSearchLobbyWidget.generated.h"

class USFOSSGameInstance;
class UListView;
class UTextBlock;
class UCheckBox;

class UCommonButtonBase;

UCLASS()
class SF_API USFSearchLobbyWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

protected:
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

public:
    //==============================UI 위젯==============================
    UPROPERTY(meta = (BindWidget))
    UListView* SessionListView; //세션 표시용 ListView

    UPROPERTY(meta = (BindWidget))
    UCheckBox* PasswordProtectedCheckBox; //비밀번호 방 포함 여부 체크박스

    UPROPERTY(meta = (BindWidget))
    UCommonButtonBase* RefreshButton; //새로고침 버튼

    UPROPERTY(meta = (BindWidget))
    UCommonButtonBase* CreateRoomButton; //방 생성 버튼

    UPROPERTY(meta = (BindWidget))
    UCommonButtonBase* CancelButton; //뒤로 가기 버튼

    UPROPERTY(meta = (BindWidget))
    UTextBlock* StatusText; //현재 세션 상태 텍스트(나중에는 빼도 될듯)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> CreateRoomWidgetClass; //방 생성 위젯 클래스
    //====================================================================

    //==============================콜백 함수===============================
    UFUNCTION()
    void OnRefreshButtonClicked(); //새로고침 클릭

    UFUNCTION()
    void OnCreateRoomButtonClicked(); //방 생성 클릭

    UFUNCTION()
    void OnCancelButtonClicked();

    UFUNCTION()
    void OnPasswordFilterChanged(bool bIsChecked); //비밀번호 필터 변경

    UFUNCTION()
    void OnSessionsUpdated(const TArray<FSessionInfo>& Sessions); //세션 리스트 업데이트
    //====================================================================

private:
    UPROPERTY()
    USFOSSGameInstance* GameInstance; //GameInstance 참조

    void RefreshSessionList(); //세션 검색 요청
    void CreateRoomUI(); //방 생성 UI 표시

public:
    // 최소 슬롯 개수 Ex) 화면에 표시될 최소 방 목록 = 가짜 방 목록 포함
    UPROPERTY(BlueprintReadWrite, Category = "UI")
    int32 MinSlots = 10;
};