#include "SFPlayFabSubsystem.h"
#include "JsonObjectConverter.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "PlayFabRuntimeSettings.h"


//===================초기화 및 PlayFab 로그인 실행=======================
void USFPlayFabSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    //Title ID 설정
    GetMutableDefault<UPlayFabRuntimeSettings>()->TitleId = TEXT("1508CD");

    UE_LOG(LogTemp, Warning, TEXT("[PlayFabSubsystem] Initialized"));
    LoginToPlayFab();
}
//====================================================================

//=========================Subsystem 정리==============================
void USFPlayFabSubsystem::Deinitialize()
{
    Super::Deinitialize();
}
//====================================================================

//===========================PlayFab 로그인============================
void USFPlayFabSubsystem::LoginToPlayFab()
{
    //OnlineSubsystem 확인 (Steam이어야 함)
    IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
    if (OnlineSub && OnlineSub->GetSubsystemName() == STEAM_SUBSYSTEM)
    {
        IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
        if (Identity.IsValid() && Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn)
        {
            //Steam 인증 토큰(세션 티켓) 가져오기 (이미 hex 문자열 형태임)
            FString SteamTicket = Identity->GetAuthToken(0);
            if (!SteamTicket.IsEmpty())
            {
                //PlayFab LoginWithSteam 요청 구성
                PlayFab::ClientModels::FLoginWithSteamRequest Request;
                Request.TitleId = GetMutableDefault<UPlayFabRuntimeSettings>()->TitleId;
                Request.SteamTicket = SteamTicket;
                Request.CreateAccount = true;
                Request.TicketIsServiceSpecific = false; //세션 티켓 사용 시 false
                //PlayFab 로그인 호출
                PlayFab::UPlayFabClientAPI::FLoginWithSteamDelegate Success;
                PlayFab::FPlayFabErrorDelegate Error;
                Success.BindUObject(this, &USFPlayFabSubsystem::OnLoginSuccess);
                Error.BindUObject(this, &USFPlayFabSubsystem::OnLoginError);
                PlayFab::UPlayFabClientAPI().LoginWithSteam(Request, Success, Error);
                return;
            }
        }
    }
    // Steam 인증에 실패한 경우 처리 (로깅 등)
    UE_LOG(LogTemp, Error, TEXT("[PlayFabSubsystem] Steam login failed or not available"));
}

void USFPlayFabSubsystem::OnLoginSuccess(const PlayFab::ClientModels::FLoginResult& Result)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayFabSubsystem] Login Success"));
    LoadPlayerData();
}

void USFPlayFabSubsystem::OnLoginError(const PlayFab::FPlayFabCppError& Error)
{
    UE_LOG(LogTemp, Error, TEXT("[PlayFabSubsystem] Login Failed: %s"),
        *Error.GenerateErrorReport());
}
//====================================================================

//========================세이브 데이터 관리=============================
//골드(영구재화)
void USFPlayFabSubsystem::AddGold(int32 Amount)
{
    PlayerStats.Gold += Amount;

    UE_LOG(LogTemp, Warning, TEXT("[PlayFabSubsystem] Gold Updated: %d"), PlayerStats.Gold);
}
//====================================================================

//===========================데이터 저장================================
void USFPlayFabSubsystem::SavePlayerData()
{
    FString JsonString;
    FJsonObjectConverter::UStructToJsonObjectString(PlayerStats, JsonString);

    PlayFab::ClientModels::FUpdateUserDataRequest Request;
    Request.Data.Add(TEXT("SaveJson"), JsonString);

    PlayFab::UPlayFabClientAPI::FUpdateUserDataDelegate Success;
    PlayFab::FPlayFabErrorDelegate Error;

    Success.BindUObject(this, &USFPlayFabSubsystem::OnDataSaved);
    Error.BindUObject(this, &USFPlayFabSubsystem::OnSaveError);

    PlayFab::UPlayFabClientAPI().UpdateUserData(Request, Success, Error);
}

void USFPlayFabSubsystem::OnDataSaved(const PlayFab::ClientModels::FUpdateUserDataResult& Result)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayFabSubsystem] Save Success"));
}

void USFPlayFabSubsystem::OnSaveError(const PlayFab::FPlayFabCppError& Error)
{
    UE_LOG(LogTemp, Error, TEXT("[PlayFabSubsystem] Save Failed: %s"),
        *Error.GenerateErrorReport());
}
//====================================================================

//============================데이터 로드===============================
void USFPlayFabSubsystem::LoadPlayerData()
{
    PlayFab::ClientModels::FGetUserDataRequest Request;

    PlayFab::UPlayFabClientAPI::FGetUserDataDelegate Success;
    PlayFab::FPlayFabErrorDelegate Error;

    Success.BindUObject(this, &USFPlayFabSubsystem::OnDataLoaded);
    Error.BindUObject(this, &USFPlayFabSubsystem::OnSaveError);

    PlayFab::UPlayFabClientAPI().GetUserData(Request, Success, Error);
}

void USFPlayFabSubsystem::OnDataLoaded(const PlayFab::ClientModels::FGetUserDataResult& Result)
{
    if (const PlayFab::ClientModels::FUserDataRecord* Record = Result.Data.Find(TEXT("SaveJson")))
    {
        FString JsonString = Record->Value;

        UE_LOG(LogTemp, Warning, TEXT("[PlayFabSubsystem] Raw SaveJson: %s"), *JsonString);

        if (!FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &PlayerStats, 0, 0))
        {
            UE_LOG(LogTemp, Error, TEXT("[PlayFabSubsystem] JSON Parse Failed"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayFabSubsystem] No Save Data"));
    }
}
//====================================================================