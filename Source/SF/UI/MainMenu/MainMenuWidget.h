#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Common/CommonButtonBase.h"

#include "MainMenuWidget.generated.h"

UCLASS()
class SF_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

protected:
	// ------------- Button -------------
	UPROPERTY(meta = (BindWidget))
	UCommonButtonBase* Btn_NewGame;

	UPROPERTY(meta = (BindWidget))
	UCommonButtonBase* Btn_SearchMatch;

	UPROPERTY(meta = (BindWidget))
	UCommonButtonBase* Btn_Options;

	UPROPERTY(meta = (BindWidget))
	UCommonButtonBase* Btn_Credits;

	UPROPERTY(meta = (BindWidget))
	UCommonButtonBase* Btn_Quit;

	// ------------- Game Flow -------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Flow")
	TSoftObjectPtr<UWorld> LobbyMapAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> CreateGameWidgetClass;

	// ------------- Function -------------
	UFUNCTION()
	void OnNewGameClicked();

	UFUNCTION()
	void OnSearchMatchClicked();

	UFUNCTION()
	void OnOptionsClicked();

	UFUNCTION()
	void OnCreditsClicked();
	
	UFUNCTION()
	void OnQuitClicked();
	
};
