#include "SFOSSGameInstance.h"

#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"
#include "SFPlayFabSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

static const FName GAME_SESSION_NAME(TEXT("Game Session"));

//================================생성자 & 초기화===================================
USFOSSGameInstance::USFOSSGameInstance()
{
}

void USFOSSGameInstance::Init()
{
	Super::Init();

	//OS 유효성 확인
	OnlineSubsystem = IOnlineSubsystem::Get();
	if (!OnlineSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("[Init] Online Subsystem not found"));
		return;
	}

	//현재 OS의 인터페이스를 가져옴
	SessionInterface = OnlineSubsystem->GetSessionInterface();
	
	//Steam 로그인 상태 확인
	if (IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
	{
		const ELoginStatus::Type Status = Identity->GetLoginStatus(0);
		bIsLoggedIn = (Status == ELoginStatus::LoggedIn);
		PlayerName = Identity->GetPlayerNickname(0);
	}
	else
	{
		bIsLoggedIn = true;
	}

	//OSS 상태 확인(Steam인지 NULL인지)
	if (IOnlineSubsystem::Get())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Init] Current OSS: %s"), *IOnlineSubsystem::Get()->GetSubsystemName().ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Init] OnlineSubsystem Get Failed"));
	}
}
//===============================================================================

//=================================세션 생성=======================================
//세션 생성
void USFOSSGameInstance::CreateGameSession(const FString& RoomName, bool bProtected, int32 MaxPlayers)
{
	//유효성 검사
	if (!bIsLoggedIn || !SessionInterface.IsValid())
	{
		OnCreateSessionComplete_Sig.Broadcast(false, TEXT("[CreateGameSession] Not login or No session interface"));
		return;
	}

	//기존 세션이 존재하면 삭제 후 생성
	if (SessionInterface->GetNamedSession(GAME_SESSION_NAME))
	{
		bWantsCreateAfterDestroy = true;

		PendingCreateSettings = MakeShared<FOnlineSessionSettings>();
		PendingCreateSettings->bIsLANMatch = false;
		PendingCreateSettings->bIsDedicated = false;
		PendingCreateSettings->NumPublicConnections = MaxPlayers;
		PendingCreateSettings->bShouldAdvertise = true;
		PendingCreateSettings->bAllowJoinInProgress = true;
		PendingCreateSettings->bAllowJoinViaPresence = true;
		PendingCreateSettings->bUsesPresence = true;
		PendingCreateSettings->bUseLobbiesIfAvailable = true;
		PendingCreateSettings->BuildUniqueId = 1;

		PendingCreateSettings->Set(TEXT("PASSWORD"), SessionPassword, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		PendingCreateSettings->Set(TEXT("ROOM_NAME"), RoomName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		PendingCreateSettings->Set(TEXT("PROTECTED"), bProtected, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

		SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &USFOSSGameInstance::OnDestroySessionComplete));
		SessionInterface->DestroySession(GAME_SESSION_NAME);
		return;
	}

	//없을 경우 바로 생성
	PendingCreateSettings = MakeShared<FOnlineSessionSettings>();
	PendingCreateSettings->bIsLANMatch = false;
	PendingCreateSettings->bIsDedicated = false;
	PendingCreateSettings->NumPublicConnections = MaxPlayers;
	PendingCreateSettings->bShouldAdvertise = true;
	PendingCreateSettings->bAllowJoinInProgress = true;
	PendingCreateSettings->bAllowJoinViaPresence = true;
	PendingCreateSettings->bUsesPresence = true;
	PendingCreateSettings->bUseLobbiesIfAvailable = true;
	PendingCreateSettings->BuildUniqueId = 1;

	PendingCreateSettings->Set(TEXT("PASSWORD"), SessionPassword, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	PendingCreateSettings->Set(TEXT("ROOM_NAME"), RoomName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	PendingCreateSettings->Set(TEXT("PROTECTED"), bProtected, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	//실제 생성 호출
	InternalCreateSession();
}

//실질적으로 세션이 생성되는 위치
void USFOSSGameInstance::InternalCreateSession()
{
	if (!SessionInterface.IsValid() || !PendingCreateSettings.IsValid())
	{
		OnCreateSessionComplete_Sig.Broadcast(false, TEXT("[InternalCreateSession] No session interface or settings"));
		return;
	}

	SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &USFOSSGameInstance::OnCreateSessionComplete));

	if (!SessionInterface->CreateSession(0, GAME_SESSION_NAME, *PendingCreateSettings))
	{
		OnCreateSessionComplete_Sig.Broadcast(false, TEXT("[InternalCreateSession] CreateSession call failed"));
	}
}

//세션 생성 완료 시 호출
void USFOSSGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CreateSessionComplete] Session created successfully: %s"), *SessionName.ToString());
		CurrentSessionName = SessionName.ToString();
		OnCreateSessionComplete_Sig.Broadcast(true, TEXT("Session created"));

		//Listen 서버로 레벨 오픈
		LoadWaitingLevel_AsHost();
		
		if (USFPlayFabSubsystem* PF = GetSubsystem<USFPlayFabSubsystem>())
		{
			PF->ResetPermanentUpgradeForNewGameSession();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CreateSessionComplete] Failed to create session"));
		OnCreateSessionComplete_Sig.Broadcast(false, TEXT("Failed to create session"));
	}

	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegates(this);
	}

	PendingCreateSettings.Reset();
	bWantsCreateAfterDestroy = false;
}
//================================================================================

//==================================세션 종료 & 파괴=================================
//세션 종료
void USFOSSGameInstance::Shutdown()
{
	//유효성 검사 및 세션 종료
	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(GAME_SESSION_NAME))
	{
		SessionInterface->DestroySession(GAME_SESSION_NAME);
	}

	Super::Shutdown();
}

//세션 파괴
void USFOSSGameInstance::DestroyMySession()
{
	if (!SessionInterface.IsValid()) return;

	SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &USFOSSGameInstance::OnDestroySessionComplete));

	//실제 파괴 호출
	SessionInterface->DestroySession(GAME_SESSION_NAME);
}

//파괴 시 호출
void USFOSSGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("Session destroyed"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Destroy session failed"));
	}

	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegates(this);
	}
	
	if (bWantsCreateAfterDestroy)
	{
		InternalCreateSession();
	}
}
//================================================================================

//===================================세션 찾기======================================
void USFOSSGameInstance::FindSessions(bool bIncludePasswordProtected)
{
	//유효성 검사
	if (!bIsLoggedIn || !SessionInterface.IsValid())
	{
		OnSessionsUpdated.Broadcast(AvailableSessions);
		return;
	}

	//Host인 상태에서 세션 검색 충돌 방지(이럴 경우 없긴 함)
	if (SessionInterface->GetNamedSession(GAME_SESSION_NAME))
	{
		UE_LOG(LogTemp, Warning, TEXT("[FindSessions] Destroying existing session before search"));
		SessionInterface->DestroySession(GAME_SESSION_NAME);
	}

	//비밀 방 표시 여부
	bShowPasswordProtected = bIncludePasswordProtected;
	
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery = false;
	SessionSearch->MaxSearchResults = 999999;
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	
	SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &USFOSSGameInstance::OnFindSessionsComplete));

	//실제 검색 호출
	SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
}

//세션 찾기 완료 시 호출
void USFOSSGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	//유효성 검사
	if (!SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[FindSessionsComplete] SessionSearch invalid"));
		OnSessionsUpdated.Broadcast(AvailableSessions);
		return;
	}

	//기존 방 리스트 초기화
	AvailableSessions.Empty();

	//검색 결과 반복 처리
	for (const auto& SearchResult : SessionSearch->SearchResults)
	{
		FSessionInfo Info;
		Info.HostName = SearchResult.Session.OwningUserName;
		Info.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
		Info.CurrentPlayers = Info.MaxPlayers - SearchResult.Session.NumOpenPublicConnections;

		SearchResult.Session.SessionSettings.Get(TEXT("ROOM_NAME"), Info.RoomName);

		bool bProtected = false;
		SearchResult.Session.SessionSettings.Get(TEXT("PROTECTED"), bProtected);
		Info.bIsPasswordProtected = bProtected;

		if (Info.bIsPasswordProtected && !bShowPasswordProtected)
		{
			continue;
		}

		Info.SearchResult = SearchResult;
		AvailableSessions.Add(MoveTemp(Info));
	}

	//Info 사용해서 UI 업데이트
	OnSessionsUpdated.Broadcast(AvailableSessions);

	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegates(this);
	}
}
//=================================================================================

//===================================세션 입장=======================================
void USFOSSGameInstance::JoinGameSession(int32 SessionIndex, const FString& InputPasswordOptional)
{
	//유효성 검사
	if (!bIsLoggedIn || !SessionInterface.IsValid() || SessionIndex < 0 || SessionIndex >= AvailableSessions.Num())
	{
		OnJoinSessionComplete_Sig.Broadcast(false, TEXT("Invalid session index or not logged in"));
		return;
	}

	//UI에서 입력된 비밀번호 저장
	LastInputPassword = InputPasswordOptional;

	SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &USFOSSGameInstance::OnJoinSessionComplete));

	//실제 Join 호출
	if (!SessionInterface->JoinSession(0, GAME_SESSION_NAME, AvailableSessions[SessionIndex].SearchResult))
	{
		OnJoinSessionComplete_Sig.Broadcast(false, TEXT("JoinSession call failed"));
	}
}

void USFOSSGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	//유효성 검사
	if (!SessionInterface.IsValid())
	{
		OnJoinSessionComplete_Sig.Broadcast(false, TEXT("No session interface"));
		return;
	}

	//Join 성공 시 실행
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		FString ConnectString;
		//접속 주소 얻기
		if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
		{
			FString FinalConnectString = ConnectString;

			//입력된 비밀번호가 존재하면 ConnectString에 추가
			if (!LastInputPassword.IsEmpty())
			{
				FinalConnectString += FString::Printf(TEXT("?Password=%s"), *LastInputPassword);
			}

			//PlayerController 유효성 검사 후 접속 시도
			APlayerController* PC = GetFirstLocalPlayerController();
			if (PC)
			{
				UE_LOG(LogTemp, Log, TEXT("ClientTravel to %s"), *ConnectString);
				PC->ClientTravel(FinalConnectString, TRAVEL_Absolute);
				OnJoinSessionComplete_Sig.Broadcast(true, TEXT("Joined session"));
			}
			else
			{
				OnJoinSessionComplete_Sig.Broadcast(false, TEXT("No PlayerController"));
				SessionInterface->DestroySession(SessionName);
			}
		}
		else
		{
			OnJoinSessionComplete_Sig.Broadcast(false, TEXT("GetResolvedConnectString failed"));
			SessionInterface->DestroySession(SessionName);
		}
	}
	else
	{
		OnJoinSessionComplete_Sig.Broadcast(false, FString::Printf(TEXT("Join failed: %d"), (int32)Result));
		SessionInterface->DestroySession(SessionName);
	}

	SessionInterface->ClearOnJoinSessionCompleteDelegates(this);
}
//=================================================================================


//====================================레벨 로드======================================
//메인메뉴
void USFOSSGameInstance::LoadMainMenu()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::OpenLevel(World, FName(TEXT("L_TestMainMenu_HJY")));
	}
}

//로비
void USFOSSGameInstance::LoadLobby()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::OpenLevel(World, FName(TEXT("L_TestSearchLobby_HJY")));
	}
}

//실제 게임 시작 전 대기실
void USFOSSGameInstance::LoadWaitingLevel_AsHost()
{
	if (UWorld* World = GetWorld())
	{
		// TODO : 하드 코딩 되어있는 LobbyMapName 리팩토링
		//listen 옵션으로 열어 리슨 서버 시작
		UGameplayStatics::OpenLevel(World, FName(TEXT("L_Lobby_LJG")), true, TEXT("listen"));
	}
}

//검색 결과 초기화(사용 안하는 중)
void USFOSSGameInstance::ClearSessionSearch()
{
	AvailableSessions.Empty();
	SessionSearch.Reset();
}
//=================================================================================