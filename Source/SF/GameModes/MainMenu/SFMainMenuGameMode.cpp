#include "SFMainMenuGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "System/SFOSSGameInstance.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "Engine/World.h"

//==================================================================================
//멀티플레이 테스트용 Game Mode, 추후에 바뀔 수도 있음
//==================================================================================
ASFMainMenuGameMode::ASFMainMenuGameMode()
{
}

void ASFMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	//Timer를 사용해서 한 프레임 후에 위젯 생성
	GetWorld()->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &ASFMainMenuGameMode::CreateWidgetWithDelay)
	);

	USFOSSGameInstance* GI = Cast<USFOSSGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GI)
	{
		SessionPassword = GI->GetSessionPassword();
		bIsSessionPasswordProtected = GI->IsSessionPasswordProtected();
	}
}

void ASFMainMenuGameMode::CreateWidgetWithDelay()
{
	// PlayerController 가져오기
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerController is NULL!"));
		return;
	}

	//마우스 커서 표시
	PlayerController->bShowMouseCursor = true;
	PlayerController->DefaultMouseCursor = EMouseCursor::Default;

	//현재 레벨 이름
	FString LevelName = GetWorld()->GetMapName();
	LevelName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	UE_LOG(LogTemp, Warning, TEXT("Current Level: %s"), *LevelName);

	//레벨별로 설정된 위젯 클래스 사용
	TSubclassOf<UUserWidget> WidgetClassToUse;

	if (LevelName.Contains(TEXT("L_TestMainMenu_HJY")))
	{
		WidgetClassToUse = MainMenuWidgetClass;
	}
	else if (LevelName.Contains(TEXT("L_TestSearchLobby_HJY")))
	{
		WidgetClassToUse = LobbyWidgetClass;
	}
	else if (LevelName.Contains(TEXT("L_TestLobby_KMY")))
	{
		WidgetClassToUse = WaitingRoomWidgetClass;
	}

	//위젯 클래스 확인
	if (!WidgetClassToUse)
	{
		UE_LOG(LogTemp, Error, TEXT("Widget class not set! Level: %s"), *LevelName);
		return;
	}

	//위젯 생성
	CurrentWidget = CreateWidget<UUserWidget>(PlayerController, WidgetClassToUse);
	
	if (!CurrentWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create widget!"));
		return;
	}

	//뷰포트에 추가
	CurrentWidget->AddToViewport(0);

	//입력 모드 설정
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(CurrentWidget->TakeWidget());
	PlayerController->SetInputMode(InputMode);

	UE_LOG(LogTemp, Warning, TEXT("Widget created and displayed successfully!"));
}

void ASFMainMenuGameMode::PreLogin(const FString& Options, const FString& Address, 
							const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	//세션이 비밀번호로 보호되어 있다면 Options에서 "Password" 키 추출
	if (bIsSessionPasswordProtected)
	{
		FString InputPassword = UGameplayStatics::ParseOption(Options, TEXT("Password"));
		if (!InputPassword.IsEmpty())
		{
			if (!InputPassword.Equals(SessionPassword))
			{
				//비밀번호 불일치 -> 접속 거부
				ErrorMessage = TEXT("InvalidPassword");  // 또는 커스텀 메시지
				return;
			}
		}
		else
		{
			//비밀번호가 필요한데 입력이 누락된 경우 처리
			ErrorMessage = TEXT("MissingPassword");
			return;
		}
	}

	//비밀번호가 없거나 일치하는 경우, 기본 로직 실행하여 접속 허용
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}