#include "UI/MainMenu/MainMenuWidget.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(Btn_NewGame))
	{
		Btn_NewGame->OnButtonClickedDelegate.AddDynamic(this, &UMainMenuWidget::OnNewGameClicked);
	}

	if (IsValid(Btn_SearchMatch))
	{
		Btn_SearchMatch->OnButtonClickedDelegate.AddDynamic(this, &UMainMenuWidget::OnSearchMatchClicked);
	}

	if (IsValid(Btn_Options))
	{
		Btn_Options->OnButtonClickedDelegate.AddDynamic(this, &UMainMenuWidget::OnOptionsClicked);
	}

	if (IsValid(Btn_Credits))
	{
		Btn_Credits->OnButtonClickedDelegate.AddDynamic(this, &UMainMenuWidget::OnCreditsClicked);
	}
	
	if (IsValid(Btn_Quit))
	{
		Btn_Quit->OnButtonClickedDelegate.AddDynamic(this, &UMainMenuWidget::OnQuitClicked);
	}
}

void UMainMenuWidget::OnNewGameClicked()
{
	if (!CreateGameWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateGameWidgetClass 가 UMainMenuWidget BP에서 설정되지 않았습니다."));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		UUserWidget* CreateGameWidget = CreateWidget<UUserWidget>(World, CreateGameWidgetClass);

		if (CreateGameWidget)
		{
			CreateGameWidget->AddToViewport(100);   // 맨 위에서 생성되도록
			UE_LOG(LogTemp, Warning, TEXT("CreateGame 위젯 생성 및 출력 완료"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("CreateGameWidget 생성 실패"));
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("세션 생성 창 출력 요청"));
}

void UMainMenuWidget::OnSearchMatchClicked()
{
	/*if (!LobbyMapAsset.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("LobbyMapAsset 이 UMainMenuWidget BP에서 설정되지 않았습니다."));
		return;
	}*/

	// 1. 맵 이름 가져오기
	FString MapName = LobbyMapAsset.GetLongPackageName();
	
	// 2. 맵 이동 (openlevel)
	if (UWorld* World = GetWorld())
	{
		if (!MapName.IsEmpty())
		{
			// 절대 경로로 맵 이동
			UGameplayStatics::OpenLevel(World, FName(*MapName));

			UE_LOG(LogTemp, Warning, TEXT("맵 이동 요청 : %s"), *MapName);
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("SearchMatch 버튼 클릭: 세션 찾기 맵으로 이동 시도"));
}

void UMainMenuWidget::OnOptionsClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Options 버튼 클릭: 옵션 창으로 이동 시도"));
}

void UMainMenuWidget::OnCreditsClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Credits 버튼 클릭: 크레딧 창으로 이동 시도"));
}

void UMainMenuWidget::OnQuitClicked()
{
	if (UWorld* World = GetWorld())
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);

		if (PC)
		{
			UKismetSystemLibrary::QuitGame(World, PC, EQuitPreference::Quit, false);
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("Quit 버튼 클릭 : 게임 종료 시도"));
}
