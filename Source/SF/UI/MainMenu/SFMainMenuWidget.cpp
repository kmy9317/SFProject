#include "SFMainMenuWidget.h"

#include "System/SFOSSGameInstance.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void USFMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//==============================이벤트 바인딩==============================
	if (StartGameButton)
	{
		StartGameButton->OnClicked.AddDynamic(this, &USFMainMenuWidget::OnStartGameButtonClicked);
	}

	if (QuitGameButton)
	{
		QuitGameButton->OnClicked.AddDynamic(this, &USFMainMenuWidget::OnQuitGameButtonClicked);
	}
	//=======================================================================
}

//==============================버튼 이벤트==============================

void USFMainMenuWidget::OnStartGameButtonClicked()
{
	USFOSSGameInstance* GameInstance = Cast<USFOSSGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GameInstance)
	{
		GameInstance->LoadLobby(); //로비로 이동
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[MainMenuWidget] GameInstance is NULL"));
	}
}

void USFMainMenuWidget::OnQuitGameButtonClicked()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, true); //게임 종료
}
//===============================================================================
