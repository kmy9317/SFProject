#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFCreateRoomWidget.generated.h"

class USFOSSGameInstance;
class UCheckBox;
class USpinBox;
class UButton;
class UEditableTextBox;
class UTextBlock;

UCLASS()
class SF_API USFCreateRoomWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

protected:
    //====================================UI 위젯======================================
    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* RoomNameInput; //방 이름 입력란

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* PasswordInput; //비밀번호 입력란

    UPROPERTY(meta = (BindWidget))
    USpinBox* MaxPlayersSpinBox; //최대 플레이어 설정

    UPROPERTY(meta = (BindWidget))
    UButton* CreateButton; //생성 버튼

    UPROPERTY(meta = (BindWidget))
    UButton* CancelButton; //취소 버튼

    UPROPERTY(meta = (BindWidget))
    UTextBlock* ErrorMessageText; //오류 메시지 표시

    UPROPERTY(meta = (BindWidget))
    UCheckBox* SecretRoomCheckBox; //비밀방 여부 체크박스
    //==================================================================================

    //====================================콜백 바인딩=====================================
    UFUNCTION()
    void OnCreateButtonClicked(); //방 생성 버튼 클릭 처리

    UFUNCTION()
    void OnCancelButtonClicked(); //방 생성 취소 처리

    UFUNCTION()
    void OnSecretRoomCheckboxChanged(bool bIsChecked); //비밀번호 입력 활성 여부

    UFUNCTION()
    void OnCreateSessionComplete(bool bWasSuccessful, const FString& Message); //세션 생성 완료 콜백
    //===================================================================================

private:
    UPROPERTY()
    USFOSSGameInstance* GameInstance; //세션 생성, 관리용 GameInstance 참조
};