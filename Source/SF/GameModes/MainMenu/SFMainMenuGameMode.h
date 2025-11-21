#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SFMainMenuGameMode.generated.h"

//==================================================================================
//멀티플레이 테스트용 Game Mode, 추후에 바뀔 수도 있음
//==================================================================================

class UUserWidget;

UCLASS()
class SF_API ASFMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASFMainMenuGameMode();

	virtual void BeginPlay() override;

	//에디터에서 위젯 클래스를 드래그 & 드롭으로 설정 가능
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> LobbyWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> WaitingRoomWidgetClass;
	
	//세션 비밀번호와 보호 여부를 관리하는 멤버 변수
	FString SessionPassword;
	bool bIsSessionPasswordProtected = false;
	
private:
	UPROPERTY()
	UUserWidget* CurrentWidget;
	
	void CreateWidgetWithDelay();

	virtual void PreLogin(
		const FString& Options,
		const FString& Address,
		const FUniqueNetIdRepl& UniqueId,
		FString& ErrorMessage) override;
};