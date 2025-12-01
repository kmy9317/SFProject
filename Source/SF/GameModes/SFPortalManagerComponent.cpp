#include "SFPortalManagerComponent.h"

#include "SFGameState.h"
#include "Actors/SFPortal.h"
#include "GameModes/SFGameMode.h"
#include "Player/SFPlayerState.h"
#include "SFLogChannels.h"
#include "Blueprint/UserWidget.h"
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
	bPortalActive = false;
	bIsPrepareToTravel = false;
    TravelDelayTime = 3.f;
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

    // 중간에 플레이어 로그아웃한 케이스 처리를 위한 바인딩
    if (ASFGameState* SFGameState = GetGameState<ASFGameState>())
    {
        SFGameState->OnPlayerRemoved.AddDynamic(this, &USFPortalManagerComponent::HandlePlayerRemoved);
    }
}

void USFPortalManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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

    // 포탈 활성화 시점에 이미 안에 있는 플레이어 처리 
    RecheckExistingOverlaps();
}

void USFPortalManagerComponent::RecheckExistingOverlaps()
{
    if (!ManagedPortal ||!bPortalActive)
    {
        return;
    }

    // 포탈 액터의 트리거 컴포넌트에 겹쳐진 모든 액터를 가져옴.
    TArray<AActor*> OverlappingActors;
    if (ManagedPortal->GetTriggerComponent())
    {
        ManagedPortal->GetTriggerComponent()->GetOverlappingActors(OverlappingActors, APawn::StaticClass());
    }

    // TODO : 포탈 엑터에서 Overlap중인 Player수 캐싱하도록 고려
    for (AActor* Actor : OverlappingActors)
    {
        if (APawn* Pawn = Cast<APawn>(Actor))
        {
            if (APlayerState* PS = Pawn->GetPlayerState())
            {
                // 이미 bIsReadyForTravel이 true라면 NotifyPlayerEnteredPortal 내부에서 무시
                NotifyPlayerEnteredPortal(PS);
            }
        }
    }
}

void USFPortalManagerComponent::NotifyPlayerEnteredPortal(APlayerState* PlayerState)
{
    if (!GetOwner()->HasAuthority() || !bPortalActive || bIsPrepareToTravel)
    {
        return;
    }

    ASFPlayerState* SFPlayerState = Cast<ASFPlayerState>(PlayerState);
    if (!SFPlayerState || SFPlayerState->IsInactive() || SFPlayerState->GetIsReadyForTravel())
    {
        return;
    }

    SFPlayerState->SetIsReadyForTravel(true);

    // 첫 번째 플레이어 진입 시 UI 활성화
    // BroadcastPortalState가 이 역할을 수행 (PortalState.bIsActive가 true로 설정됨)
    if (!PortalState.bIsActive)
    {
        BroadcastPortalState();
    }

    // 모든 플레이어 준비 상태 확인
    CheckPortalReadyAndTravel();
}

void USFPortalManagerComponent::NotifyPlayerLeftPortal(APlayerState* PlayerState)
{
    if (!GetOwner()->HasAuthority() || !bPortalActive || bIsPrepareToTravel)
    {
        return;
    }

    ASFPlayerState* SFPlayerState = Cast<ASFPlayerState>(PlayerState);
    if (!SFPlayerState || SFPlayerState->IsInactive() || !SFPlayerState->GetIsReadyForTravel())
    {
        return;
    }

    SFPlayerState->SetIsReadyForTravel(false);
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

void USFPortalManagerComponent::CheckPortalReadyAndTravel()
{
    if (!GetOwner()->HasAuthority() || !bPortalActive || bIsPrepareToTravel)
    {
        return; 
    }
    
    const int32 RequiredCount = GetRequiredPlayerCount();
    if (RequiredCount <= 0)
    {
        return;
    }
    
    int32 ReadyCount = GetPlayersReadyCount();

    // 아직 모든 플레이어가 준비되지 않음
    if (ReadyCount < RequiredCount)
    {
        return; 
    }

    // 카운트다운이 이미 시작되지 않았는지 확인
    if (TravelTimerHandle.IsValid())
    {
        return;
    }

    bIsPrepareToTravel = true;
    
    // 카운트다운 시작
    PortalState.TravelCountdown = TravelDelayTime;
    
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

    FSFPortalStateMessage NewState;
    NewState.bIsActive = bPortalActive;
    NewState.TotalPlayerCount = GetRequiredPlayerCount();
    NewState.TravelCountdown = PortalState.TravelCountdown;
    PortalState = NewState;

    // Listen server 전용
    OnRep_PortalStateChanged();
}

int32 USFPortalManagerComponent::GetPlayersReadyCount()
{
    int32 ReadyCount = 0;
    const AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
    for (APlayerState* PS : GameState->PlayerArray)
    {
        if (const ASFPlayerState* SFPlayerState = Cast<ASFPlayerState>(PS))
        {
            if (SFPlayerState->GetIsReadyForTravel())
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

void USFPortalManagerComponent::HandlePlayerRemoved(APlayerState* PlayerState)
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    if (!bPortalActive || !PortalState.bIsActive || bIsPrepareToTravel)
    {
        return;
    }

    // 나간 플레이어 덕분에 남은 인원이 모두 준비 완료가 되었는지 확인
    CheckPortalReadyAndTravel();
    
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

void USFPortalManagerComponent::OnRep_PortalStateChanged()
{
    UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
    
    // 각 클라이언트에서 UI는 이 메시지를 수신하여 자신을 표시/숨김/카운트다운
    MessageSubsystem.BroadcastMessage(SFGameplayTags::Message_Portal_StateChanged, PortalState);

    if (PortalState.TravelCountdown > 0.f && ManagedPortal)
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
