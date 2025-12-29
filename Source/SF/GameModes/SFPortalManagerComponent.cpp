#include "SFPortalManagerComponent.h"

#include "SFGameState.h"
#include "Actors/SFPortal.h"
#include "GameModes/SFGameMode.h"
#include "Player/SFPlayerState.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"
#include "Messages/SFMessageGameplayTags.h"
#include "System/SFLoadingScreenSubsystem.h"

USFPortalManagerComponent::USFPortalManagerComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USFPortalManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bPortalActive);
	DOREPLIFETIME(ThisClass, PortalState);
}

void USFPortalManagerComponent::BeginPlay()
{
    Super::BeginPlay();
    
    if (GetOwner()->HasAuthority())
    {
        // 중간에 플레이어 로그아웃한 케이스 처리를 위한 바인딩
        if (ASFGameState* SFGameState = GetGameState<ASFGameState>())
        {
            SFGameState->OnPlayerRemoved.AddDynamic(this, &USFPortalManagerComponent::HandlePlayerRemoved);
        }

        if (UGameplayMessageSubsystem::HasInstance(this))
        {
            UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
            DeadStateListenerHandle = MessageSubsystem.RegisterListener(SFGameplayTags::Message_Player_DeadStateChanged,this, &ThisClass::OnPlayerDeadStateChanged);
        }
    }
}

void USFPortalManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearAllTimersForObject(this);

        if (UGameplayMessageSubsystem::HasInstance(this))
        {
            UGameplayMessageSubsystem::Get(World).UnregisterListener(DeadStateListenerHandle);
        }
    }
    
    if (ASFGameState* SFGameState = GetGameState<ASFGameState>())
    {
        SFGameState->OnPlayerRemoved.RemoveAll(this);
    }
    Super::EndPlay(EndPlayReason);
}

void USFPortalManagerComponent::ActivatePortal()
{
    if (!GetOwner()->HasAuthority() || bPortalActive)
    {
        return;
    }

    bPortalActive = true;
    OnRep_PortalActive();
}

void USFPortalManagerComponent::RegisterPortal(ASFPortal* Portal)
{
    if (!Portal)
    {
        return;
    }

    ManagedPortal = Portal;
    
    // 현재 활성화 상태에 맞춰 Portal 업데이트
    ManagedPortal->SetPortalEnabled(bPortalActive);
}

void USFPortalManagerComponent::UnregisterPortal(ASFPortal* Portal)
{
    if (Portal && ManagedPortal == Portal)
    {
        ManagedPortal = nullptr;
    }
}

void USFPortalManagerComponent::TogglePlayerReady(APlayerState* PlayerState)
{
    if (!GetOwner()->HasAuthority() || !PlayerState)
    {
        return;
    }

    if (!bPortalActive)
    {
        return;
    }

    if (bIsTravelCountdownActive)
    {
        return;
    }

    ASFPlayerState* SFPS = Cast<ASFPlayerState>(PlayerState);
    if (!SFPS || SFPS->IsInactive())
    {
        return;
    }

    if (SFPS->IsDead())
    {
        return;
    }

    const bool bWasReady = SFPS->GetIsReadyForTravel();
    const bool bNewReadyState = !bWasReady;
    SFPS->SetIsReadyForTravel(bNewReadyState);

    // 첫 번째 Ready (PortalState.bIsActive로 판단)
    if (bNewReadyState && !PortalState.bIsActive)
    {
        OnFirstPlayerReady();
    }

    CheckAllPlayersReady();
}

void USFPortalManagerComponent::OnFirstPlayerReady()
{
    if (PortalState.bIsActive)
    {
        return;
    }
    
    PortalState.bIsActive = true;
    PortalState.TotalPlayerCount = GetRequiredPlayerCount();
    PortalState.TravelCountdown = ForceTimeLimit;
    
    // Listen Server 로컬 처리
    OnRep_PortalState();

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            ForceTimerHandle,
            this,
            &ThisClass::ExecuteTravel,
            ForceTimeLimit,
            false
        );
    }
}

void USFPortalManagerComponent::CheckAllPlayersReady()
{
    if (!ManagedPortal || bIsTravelCountdownActive)
    {
        return;
    }

    int32 ReadyCount = GetReadyPlayerCount();
    int32 RequiredCount = GetRequiredPlayerCount();

    // 살아있는 플레이어가 없으면 Travel 하지 않음 (전멸 상태)
    if (RequiredCount <= 0)
    {
        return;
    }

    if (ReadyCount >= RequiredCount)
    {
        StartTravelCountdown();
    }
}

void USFPortalManagerComponent::StartTravelCountdown()
{
    if (bIsTravelCountdownActive)
    {
        return;
    }

    bIsTravelCountdownActive = true;

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // 강제 타이머 정지
    World->GetTimerManager().ClearTimer(ForceTimerHandle);

    PortalState.TravelCountdown = TravelDelayTime;
    BroadcastPortalState();

    World->GetTimerManager().SetTimer(
        TravelTimerHandle,
        this,
        &ThisClass::ExecuteTravel,
        TravelDelayTime,
        false
    );
}

void USFPortalManagerComponent::ExecuteTravel()
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    if (ASFGameMode* SFGameMode = GetGameMode<ASFGameMode>())
    {
        if (ManagedPortal && !ManagedPortal->GetNextStageLevel().IsNull())
        {
            SFGameMode->RequestTravelToNextStage(ManagedPortal->GetNextStageLevel());
        }
    }
}

void USFPortalManagerComponent::BroadcastPortalState()
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    PortalState.TotalPlayerCount = GetRequiredPlayerCount();

    // Listen server 전용
    OnRep_PortalState();
}

int32 USFPortalManagerComponent::GetReadyPlayerCount() const
{
    int32 ReadyCount = 0;
    const AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
    for (APlayerState* PS : GameState->PlayerArray)
    {
        if (const ASFPlayerState* SFPS = Cast<ASFPlayerState>(PS))
        {
            // Dead가 아니고 Ready인 플레이어만 카운트
            if (!SFPS->IsDead() && SFPS->GetIsReadyForTravel())
            {
                ReadyCount++;
            }
        }
    }
    return ReadyCount;
}


int32 USFPortalManagerComponent::GetRequiredPlayerCount() const
{
    const AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
    int32 AlivePlayerCount = 0;
    for (APlayerState* PS : GameState->PlayerArray)
    {
        if (const ASFPlayerState* SFPS = Cast<ASFPlayerState>(PS))
        {
            // Inactive가 아니고 Dead가 아닌 플레이어만 카운트
            if (!PS->IsInactive() && !SFPS->IsDead())
            {
                AlivePlayerCount++;
            }
        }
    }
    return AlivePlayerCount;
}

void USFPortalManagerComponent::HandlePlayerRemoved(APlayerState* PlayerState)
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    if (!bPortalActive || !PortalState.bIsActive || bIsTravelCountdownActive)
    {
        return;
    }

    // 나간 플레이어 덕분에 남은 인원이 모두 준비 완료가 되었는지 확인
    CheckAllPlayersReady();
    
    // UI 업데이트 
    BroadcastPortalState();
}

void USFPortalManagerComponent::OnRep_PortalActive()
{
    if (ManagedPortal)
    {
        ManagedPortal->SetPortalEnabled(bPortalActive);
    }
}

void USFPortalManagerComponent::OnRep_PortalState()
{
    UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
    
    // 각 클라이언트에서 UI는 이 메시지를 수신하여 자신을 표시/숨김/카운트다운
    MessageSubsystem.BroadcastMessage(SFGameplayTags::Message_Portal_StateChanged, PortalState);

    if (PortalState.bIsActive && !bLoadingScreenPreloaded)
    {
        bLoadingScreenPreloaded = true;
        
        if (ManagedPortal && !ManagedPortal->GetNextStageLevel().IsNull())
        {
            FString NextLevelPath = ManagedPortal->GetNextStageLevel().GetLongPackageName();
            FString NextLevelName = FPackageName::GetShortName(NextLevelPath);
            
            if (UGameInstance* GI = GetWorld()->GetGameInstance())
            {
                if (USFLoadingScreenSubsystem* LoadingSubsystem = GI->GetSubsystem<USFLoadingScreenSubsystem>())
                {
                    LoadingSubsystem->PreloadLoadingScreenForLevel(NextLevelName);
                }
            }
        }
    }
}

void USFPortalManagerComponent::OnPlayerDeadStateChanged(FGameplayTag Channel, const FSFPlayerDeadStateMessage& Message)
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    // Portal이 활성화 상태면 Ready 체크 재계산
    if (bPortalActive && !bIsTravelCountdownActive)
    {
        CheckAllPlayersReady();
    }

    BroadcastPortalState();
}
