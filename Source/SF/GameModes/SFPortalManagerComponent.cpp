#include "SFPortalManagerComponent.h"
#include "Actors/SFPortal.h"
#include "GameModes/SFGameMode.h"
#include "Player/SFPlayerState.h"
#include "SFLogChannels.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"
#include "Messages/SFMessageGameplayTags.h"

USFPortalManagerComponent::USFPortalManagerComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bPortalActive = false;
	bIsTraveling = false;
    TravelCountdownRemaining = -1.0f;
}

void USFPortalManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USFPortalManagerComponent, bPortalActive);
	DOREPLIFETIME(USFPortalManagerComponent, PlayersInPortal);
}

void USFPortalManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USFPortalManagerComponent::ActivatePortal()
{
    if (!GetOwner()->HasAuthority() || bPortalActive)
    {
        return;
    }

    bPortalActive = true;

    if (ManagedPortal)
    {
        // 등록된 Portal들 비주얼 업데이트
        ManagedPortal->SetPortalEnabled(true);
    }
    
    // 상태 브로드캐스트
    BroadcastPortalState();
}

void USFPortalManagerComponent::NotifyPlayerEnteredPortal(APlayerState* PlayerState)
{
    if (!GetOwner()->HasAuthority() || !bPortalActive || bIsTraveling)
    {
        return;
    }

    if (!PlayerState || PlayerState->IsInactive())
    {
        return;
    }

    // 이미 목록에 있는지 체크
    FSFPlayerSelectionInfo NewPlayerInfo = GetPlayerSelectionInfo(PlayerState);
    if (PlayersInPortal.Contains(NewPlayerInfo))
    {
        return;
    }

    // 플레이어 추가
    PlayersInPortal.Add(NewPlayerInfo);

    // 상태 브로드캐스트
    BroadcastPortalState();
    
    // 모든 플레이어 체크
    CheckPortalReadyAndTravel();
}

void USFPortalManagerComponent::NotifyPlayerLeftPortal(APlayerState* PlayerState)
{
    if (!GetOwner()->HasAuthority() || bIsTraveling)
    {
        return;
    }

    if (!PlayerState)
    {
        return;
    }

    // 플레이어 찾기
    int32 FoundIndex = PlayersInPortal.IndexOfByPredicate([PlayerState](const FSFPlayerSelectionInfo& Info)
    {
        return Info.IsForPlayer(PlayerState);
    });

    if (FoundIndex != INDEX_NONE)
    {
        FSFPlayerSelectionInfo RemovedPlayer = PlayersInPortal[FoundIndex];
        PlayersInPortal.RemoveAt(FoundIndex);

        // 상태 브로드캐스트
        BroadcastPortalState();

        // 타이머 취소
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(TravelTimerHandle);
        }
        TravelCountdownRemaining = -1.0f;
    }
}

int32 USFPortalManagerComponent::GetRequiredPlayerCount() const
{
    const AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
    
    // 유효한 PlayerState만 카운트
    int32 ValidPlayerCount = 0;
    for (APlayerState* PS : GameState->PlayerArray)
    {
        if (PS && !PS->IsInactive())
        {
            ValidPlayerCount++;
        }
    }
    return ValidPlayerCount;
}

bool USFPortalManagerComponent::AreAllPlayersInPortal() const
{
    return bPortalActive && 
           PlayersInPortal.Num() >= GetRequiredPlayerCount() && 
           !bIsTraveling && 
           GetRequiredPlayerCount() > 0;
}

void USFPortalManagerComponent::RegisterPortal(ASFPortal* Portal)
{
    if (!Portal)
    {
        return;
    }

    ManagedPortal = Portal;
    
    // 현재 활성화 상태에 맞춰 Portal 업데이트
    Portal->SetPortalEnabled(bPortalActive);
}

void USFPortalManagerComponent::UnregisterPortal(ASFPortal* Portal)
{
    if (!Portal)
    {
        return;
    }

    if (ManagedPortal == Portal)
    {
        ManagedPortal = nullptr;
        UE_LOG(LogSF, Log, TEXT("[PortalManager] Portal unregistered: %s"), *Portal->GetName());
    }
}

void USFPortalManagerComponent::CheckPortalReadyAndTravel()
{
    if (!AreAllPlayersInPortal())
    {
        return;
    }

    UE_LOG(LogSF, Warning, TEXT("[PortalManager] All players ready! Starting travel in %.1f seconds"), 
        TravelDelayTime);

    // 카운트다운 시작
    TravelCountdownRemaining = TravelDelayTime;
    
    // 상태 브로드캐스트 (카운트다운 UI 표시)
    BroadcastPortalState();

    // Travel 타이머 시작
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            TravelTimerHandle,
            this,
            &USFPortalManagerComponent::ExecuteTravel,
            TravelDelayTime,
            false
        );
    }
}

void USFPortalManagerComponent::ExecuteTravel()
{
    if (!AreAllPlayersInPortal())
    {
        UE_LOG(LogSF, Warning, TEXT("[PortalManager] Travel cancelled - not all players ready"));
        return;
    }

    bIsTraveling = true;
    
    UE_LOG(LogSF, Warning, TEXT("[PortalManager] Executing travel to next stage"));
    
    // GameMode에 Travel 요청 (다음 스테이지 정보 전달)
    if (UWorld* World = GetWorld())
    {
        if (ASFGameMode* GameMode = World->GetAuthGameMode<ASFGameMode>())
        {
            // ManagedPortal에서 다음 스테이지 레벨 가져오기
            if (ManagedPortal && !ManagedPortal->GetNextStageLevel().IsNull())
            {
                TSoftObjectPtr<UWorld> NextStageLevel = ManagedPortal->GetNextStageLevel();
                GameMode->RequestTravelToNextStage(NextStageLevel);
            }
        }
    }
}

FSFPlayerSelectionInfo USFPortalManagerComponent::GetPlayerSelectionInfo(const APlayerState* PlayerState) const
{
    if (const ASFPlayerState* SFPS = Cast<ASFPlayerState>(PlayerState))
    {
        return SFPS->GetPlayerSelection();
    }

    FSFPlayerSelectionInfo DefaultInfo;
    if (PlayerState)
    {
        DefaultInfo = FSFPlayerSelectionInfo(0, PlayerState);
    }
    return DefaultInfo;
}

void USFPortalManagerComponent::BroadcastPortalState()
{
    UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
    
    FSFPortalStateMessage Message;
    Message.bIsActive = bPortalActive;
    Message.PlayersInPortalCount = PlayersInPortal.Num();
    Message.RequiredPlayerCount = GetRequiredPlayerCount();
    Message.bReadyToTravel = AreAllPlayersInPortal();
    Message.TravelCountdown = TravelCountdownRemaining;
    
    // === 전체 플레이어의 포탈 진입 상태 구성 ===
    if (const AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>())
    {
        for (APlayerState* PS : GameState->PlayerArray)
        {
            if (!PS || PS->IsInactive())
            {
                continue;
            }
            // 이 플레이어의 SelectionInfo 가져오기 TODO : PlayerState내 FSFPlayerSelectionInfo의 로비에서 CopyProperties 및 Replication 필요해 보임 
            FSFPlayerSelectionInfo PlayerInfo = GetPlayerSelectionInfo(PS);
            
            // 포탈에 진입했는지 확인
            bool bIsInPortal = PlayersInPortal.ContainsByPredicate(
                [&PlayerInfo](const FSFPlayerSelectionInfo& Info)
                {
                    return Info == PlayerInfo;
                }
            );
            
            // 플레이어 상태 추가
            Message.PlayerStatuses.Add(FPortalPlayerStatus(PlayerInfo, bIsInPortal));
        }
    }
    MessageSubsystem.BroadcastMessage(SFGameplayTags::Message_Portal_InfoChanged, Message);
}

void USFPortalManagerComponent::OnRep_PortalActive()
{
    if (ManagedPortal)
    {
        ManagedPortal->SetPortalEnabled(bPortalActive);
    }
    
    BroadcastPortalState();
}

void USFPortalManagerComponent::OnRep_PlayersInPortal()
{
    BroadcastPortalState();
}

