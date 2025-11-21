#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "System/SFOSSGameInstance.h"
#include "SFRoomListEntryWidget.generated.h"

class USFPasswordInputWidget;
class UButton;
class UTextBlock;

UCLASS()
class SF_API USFRoomListEntryWidget : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    //======================================UI 위젯======================================
    UPROPERTY(meta=(BindWidget))
    UTextBlock* RoomNameText; //방 이름 표시

    UPROPERTY(meta=(BindWidget))
    UTextBlock* PlayerCountText; //플레이어 수 표시

    UPROPERTY(meta=(BindWidget))
    UTextBlock* HostNameText; //호스트 이름 표시

    UPROPERTY(meta=(BindWidget))
    UTextBlock* ProtectedIndicator; //비밀방 여부 표시

    UPROPERTY(meta=(BindWidget))
    UButton* JoinButton; //참가 버튼

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI")
    TSubclassOf<USFPasswordInputWidget> PasswordInputWidgetClass; //비밀번호 입력창 위젯 클래스
    //====================================================================================

    //======================================콜백 함수=======================================
    UFUNCTION() void OnJoinButtonClicked(); //참가 버튼
    UFUNCTION() void OnJoinSessionComplete(bool bWasSuccessful, const FString& Message); //세션 입장 완료 콜백
    //====================================================================================

private:
    FSessionInfo SessionInfo; //현재 표시 중인 세션 정보
    int32 SessionIndex; //Join 시 사용할 세션 인덱스

    UPROPERTY()
    USFOSSGameInstance* GameInstance; //세션 처리 GameInstance 참조

    void ShowPasswordDialog(); //비밀번호 입력 위젯 표시
};
