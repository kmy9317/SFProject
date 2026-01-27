#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "SFOSSGameInstance.generated.h"

USTRUCT(BlueprintType)

//======================================세션 정보==========================================
struct FSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString RoomName; //방 제목

	UPROPERTY(BlueprintReadOnly)
	FString HostName; //호스트 이름

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayers = 0; //현재 플레이어

	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayers = 0; //최대 플레이어

	UPROPERTY(BlueprintReadOnly)
	bool bIsPasswordProtected = false; // UI 표시용

	FOnlineSessionSearchResult SearchResult; //Join에 사용
};
//========================================================================================

//===================================델리게이트 선언========================================
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSessionsUpdated, const TArray<FSessionInfo>&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCreateSessionComplete_Sig, bool, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnJoinSessionComplete_Sig, bool, const FString&);
//========================================================================================

UCLASS()
class SF_API USFOSSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	USFOSSGameInstance();

	virtual void Init() override;
	virtual void Shutdown() override;

	//=======================================세션 생성==========================================
	UFUNCTION(BlueprintCallable, Category = "Session")
	void CreateGameSession(const FString& RoomName, bool bProtected, int32 MaxPlayers = 4);

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	//========================================================================================
	
	//======================================세션 검색===========================================
	UFUNCTION(BlueprintCallable, Category = "Session")
	void FindSessions(bool bIncludePasswordProtected = true);

	void OnFindSessionsComplete(bool bWasSuccessful);
	//========================================================================================
	
	//======================================세션 입장===========================================
	UFUNCTION(BlueprintCallable, Category = "Session")
	void JoinGameSession(int32 SessionIndex, const FString& InputPasswordOptional = TEXT(""));

	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	//========================================================================================
	
	//=======================================세션 파괴==========================================
	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroyMySession();

	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	//========================================================================================
	
	//=======================================레벨 로드==========================================
	UFUNCTION(BlueprintCallable, Category = "Level")
	void LoadMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Level")
	void LoadLobby();

	UFUNCTION(BlueprintCallable, Category = "Level")
	void LoadWaitingLevel_AsHost();
	//========================================================================================
	
	//=====================================UI 델리게이트========================================
	FOnSessionsUpdated OnSessionsUpdated; // 세션 목록 갱신시 
	FOnCreateSessionComplete_Sig OnCreateSessionComplete_Sig; // 세션 생성 완료 시 
	FOnJoinSessionComplete_Sig OnJoinSessionComplete_Sig; // 세션 참가 완료 시
	//========================================================================================
	
	//=========================================Get()==========================================
	UFUNCTION(BlueprintCallable, Category = "Session")
	const TArray<FSessionInfo>& GetAvailableSessions() const { return AvailableSessions; }

	UFUNCTION(BlueprintCallable, Category = "Session")
	bool IsLoggedIn() const { return bIsLoggedIn; }

	UFUNCTION(BlueprintCallable, Category = "Session")
	const FString& GetPlayerName() const { return PlayerName; }

	void SetSessionPassword(const FString& InPassword) {
		SessionPassword = InPassword;
		bIsSessionPasswordProtected = !InPassword.IsEmpty();
	}
	const FString& GetSessionPassword() const { return SessionPassword; }
	bool IsSessionPasswordProtected() const { return bIsSessionPasswordProtected; }

protected:

	// ========== OSS 관련 ==========
	IOnlineSubsystem* OnlineSubsystem = nullptr; // Steam or NULL 세션
	IOnlineSessionPtr SessionInterface; // 세션 인터페이스(세션 관리 기능 담당)
	TSharedPtr<FOnlineSessionSearch> SessionSearch; // 검색 결과 저장
	
	// ========== 상태 변수 ==========
	bool bIsLoggedIn = false; // 로그인 상태 
	FString PlayerName; // 플레이어 이름
	FString CurrentSessionName; // 검색된 세션 목록

	TArray<FSessionInfo> AvailableSessions;
	bool bShowPasswordProtected = true;
	
	bool bWantsCreateAfterDestroy = false;
	TSharedPtr<FOnlineSessionSettings> PendingCreateSettings;

	void PrepareSessionSettings(const FString& RoomName, bool bProtected, int32 MaxPlayers);
	void InternalCreateSession();
	void ClearSessionSearch();

private:

	// ========== 비밀번호 관련 ==========
	FString SessionPassword;                     // 세션 비밀번호
	bool bIsSessionPasswordProtected;            // 비밀번호 보호 여부
	FString LastInputPassword;                   // 참가 시 입력한 비밀번호

};