#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFMainMenuWidget.generated.h"

class UButton;

UCLASS()
class SF_API USFMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	//==============================UI 위젯============================
	UPROPERTY(meta = (BindWidget))
	UButton* StartGameButton; //게임 시작 버튼

	UPROPERTY(meta = (BindWidget))
	UButton* QuitGameButton; //게임 종료 버튼
	//================================================================

	//============================콜백 함수=============================
	UFUNCTION()
	void OnStartGameButtonClicked(); //게임 시작 클릭

	UFUNCTION()
	void OnQuitGameButtonClicked(); //게임 종료 클릭
	//=================================================================
};
