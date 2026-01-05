#include "SFGameOverManagerComponent.h"

#include "SFGameMode.h"
#include "SFLogChannels.h"
#include "SFStageManagerComponent.h"
#include "Character/Hero/SFHeroDefinition.h"
#include "Messages/SFMessageGameplayTags.h"
#include "Messages/SFPortalInfoMessages.h"
#include "Net/UnrealNetwork.h"
#include "Player/SFPlayerState.h"
#include "Player/Components/SFPlayerCombatStateComponent.h"
#include "Player/Components/SFPlayerStatsComponent.h"

USFGameOverManagerComponent::USFGameOverManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USFGameOverManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, bGameOver);
	DOREPLIFETIME(ThisClass, GameOverResult);
	DOREPLIFETIME(ThisClass, ReadyCount);
}

void USFGameOverManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		if (UGameplayMessageSubsystem::HasInstance(this))
		{
			UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
			DeadStateListenerHandle = MessageSubsystem.RegisterListener(SFGameplayTags::Message_Player_DeadStateChanged,this, &ThisClass::OnPlayerDeadStateChanged);
			DownedStateListenerHandle = MessageSubsystem.RegisterListener(SFGameplayTags::Message_Player_DownedStateChanged,this, &ThisClass::OnPlayerDownedStateChanged);
		}
	}
}

void USFGameOverManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GameOverCheckTimerHandle);
		World->GetTimerManager().ClearTimer(LobbyCountdownHandle);
		World->GetTimerManager().ClearTimer(StatsDelayHandle);
	}

	if (UGameplayMessageSubsystem::HasInstance(this))
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		MessageSubsystem.UnregisterListener(DeadStateListenerHandle);
		MessageSubsystem.UnregisterListener(DownedStateListenerHandle);
	}
	
	Super::EndPlay(EndPlayReason);
}

void USFGameOverManagerComponent::OnPlayerDeadStateChanged(FGameplayTag Channel, const FSFPlayerDeadStateMessage& Message)
{
	if (!GetOwner()->HasAuthority() || bGameOver)
	{
		return;
	}

	if (!Message.bIsDead)
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(GameOverCheckTimerHandle);
		}
		return;
	}

	// 마지막 생존자 사망 시  GameOver 메시지 전달
	if (AreAllPlayersIncapacitated())
	{
		TriggerGameOver();
		return;
	}

	ScheduleGameOverCheck();
}

void USFGameOverManagerComponent::OnPlayerDownedStateChanged(FGameplayTag Channel, const FSFPlayerDownedStateMessage& Message)
{
	if (!GetOwner()->HasAuthority() || bGameOver)
	{
		return;
	}

	// Downed에서 벗어났으면 체크 취소
	if (!Message.bIsDowned)
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(GameOverCheckTimerHandle);
		}
		return;
	}

	if (AreAllPlayersIncapacitated())
	{
		TriggerGameOver();
		return;
	}

	ScheduleGameOverCheck();
}

void USFGameOverManagerComponent::ScheduleGameOverCheck()
{
	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(GameOverCheckTimerHandle))
		{
			World->GetTimerManager().SetTimer(
				GameOverCheckTimerHandle,
				this,
				&ThisClass::CheckGameOverCondition,
				GameOverCheckDelay,
				false
			);
		}
	}
}

void USFGameOverManagerComponent::CheckGameOverCondition()
{
	if (!GetOwner()->HasAuthority() || bGameOver)
	{
		return;
	}

	if (AreAllPlayersIncapacitated())
	{
		TriggerGameOver();
	}
}

bool USFGameOverManagerComponent::AreAllPlayersIncapacitated() const
{
	const AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
    
	int32 ActivePlayerCount = 0;
	int32 IncapacitatedCount = 0;

	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (PS->IsInactive())
		{
			continue;
		}

		ActivePlayerCount++;

		const USFPlayerCombatStateComponent* CombatComp = PS->FindComponentByClass<USFPlayerCombatStateComponent>();
		if (CombatComp && CombatComp->IsIncapacitated())
		{
			IncapacitatedCount++;
		}
	}

	// 플레이어 없으면 GameOver 아님
	if (ActivePlayerCount == 0)
	{
		return false;
	}

	return IncapacitatedCount >= ActivePlayerCount;
}

void USFGameOverManagerComponent::TriggerGameOver()
{
	if (!GetOwner()->HasAuthority() || bGameOver)
	{
		return;
	}

	bGameOver = true;

	// Listen Server 로컬 처리
	OnRep_bGameOver();
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			StatsDelayHandle,
			this,
			&ThisClass::CollectAndBroadcastStats,
			StatsDisplayDelay,
			false
		);
	}
}

void USFGameOverManagerComponent::CollectAndBroadcastStats()
{
	// 통계 수집
	CollectAllPlayerStats();
	
	// 목표 시간 계산 (현재 서버 시간 + 딜레이)
	if (const AGameStateBase* GS = GetGameStateChecked<AGameStateBase>())
	{
		float CurrentServerTime = GS->GetServerWorldTimeSeconds();
		GameOverResult.TargetLobbyTime = CurrentServerTime + LobbyTransitionDelay;
	}

	bWaitingForLobbyTransition = true;

	// 통계 + 목표 시간 브로드캐스트(Listen Server 로컬 처리)
	OnRep_GameOverResult();

	// 서버 타이머: 정확한 시간에 로비 이동
	StartLobbyCountdown();
}

void USFGameOverManagerComponent::CollectAllPlayerStats()
{
	const AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	if (!GameState)
	{
		return;;
	}

	GameOverResult = FSFGameOverResult();
	
	if (USFStageManagerComponent* StageManager = GameState->FindComponentByClass<USFStageManagerComponent>())
	{
		GameOverResult.StageName = StageManager->GetCurrentStageInfo().DisplayName;
	}
	
    for (APlayerState* PS : GameState->PlayerArray)
    {
        if (PS->IsInactive())
        {
            continue;
        }

        ASFPlayerState* SFPS = Cast<ASFPlayerState>(PS);
        if (!SFPS)
        {
            continue;
        }

        FSFPlayerGameStats PlayerStats;
        
        // 플레이어 이름
        PlayerStats.PlayerName = SFPS->GetPlayerName();

        // 영웅 아이콘
        const FSFPlayerSelectionInfo& Selection = SFPS->GetPlayerSelection();
        if (USFHeroDefinition* HeroDef = Selection.GetHeroDefinition())
        {
            PlayerStats.HeroIcon = HeroDef->LoadIcon();
        }

        // StatsComponent에서 누적 통계 가져오기
        if (USFPlayerStatsComponent* StatsComp = USFPlayerStatsComponent::FindPlayerStatsComponent(PS))
        {
            PlayerStats.TotalDamageDealt = StatsComp->GetTotalDamageDealt();
            PlayerStats.DownedCount = StatsComp->GetTotalDownedCount();
            PlayerStats.ReviveCount = StatsComp->GetTotalReviveCount();
        }
        GameOverResult.PlayerStats.Add(PlayerStats);
    }

    // 데미지 순 정렬 (MVP 표시용)
    GameOverResult.PlayerStats.Sort([](const FSFPlayerGameStats& A, const FSFPlayerGameStats& B)
    {
        return A.TotalDamageDealt > B.TotalDamageDealt;
    });
}

void USFGameOverManagerComponent::StartLobbyCountdown()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			LobbyCountdownHandle,
			this,
			&ThisClass::TravelToLobby,
			LobbyTransitionDelay,
			false  
		);
	}
}

void USFGameOverManagerComponent::TravelToLobby()
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

	bWaitingForLobbyTransition = false;

	if (ASFGameMode* GameMode = GetWorld()->GetAuthGameMode<ASFGameMode>())
	{
		GameMode->RequestTravelToLobby(LobbyLevel);
	}
}


float USFGameOverManagerComponent::GetRemainingLobbyTime() const
{
	if (const AGameStateBase* GS = GetGameStateChecked<AGameStateBase>())
	{
		float CurrentTime = GS->GetServerWorldTimeSeconds();
		return FMath::Max(0.f, GameOverResult.TargetLobbyTime - CurrentTime);
	}
	return 0.f;
}

void USFGameOverManagerComponent::NotifyPlayerReadyForLobby(APlayerController* PC)
{
	if (!PC || !bWaitingForLobbyTransition)
	{
		return;
	}

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	if (ReadyPlayers.Contains(PC))
	{
		return;
	}

	CleanupInvalidReadyPlayers();

	ReadyPlayers.Add(PC);
	ReadyCount = ReadyPlayers.Num();

	// Listen Server 로컬 처리
	OnRep_ReadyCount();

	CheckAllPlayersReady();
}

void USFGameOverManagerComponent::CleanupInvalidReadyPlayers()
{
	for (auto It = ReadyPlayers.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

void USFGameOverManagerComponent::CheckAllPlayersReady()
{
	int32 TotalCount = GetTotalPlayerCount();
    
	if (TotalCount > 0 && ReadyCount >= TotalCount)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(LobbyCountdownHandle);
		}

		TravelToLobby();
	}
}

int32 USFGameOverManagerComponent::GetTotalPlayerCount() const
{
	const AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
    
	int32 Count = 0;
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (!PS->IsInactive())
		{
			Count++;
		}
	}
	return Count;
}


void USFGameOverManagerComponent::OnRep_bGameOver()
{
	if (bGameOver)
	{
		OnGameOver.Broadcast();

		if (UGameplayMessageSubsystem::HasInstance(this))
		{
			UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
			FSFGameOverMessage Message;
			MessageSubsystem.BroadcastMessage(SFGameplayTags::Message_Game_GameOver, Message);
		}
	}
}

void USFGameOverManagerComponent::OnRep_GameOverResult()
{
	// 통계 데이터 브로드캐스트 (UI에서 통계 표시)
	if (UGameplayMessageSubsystem::HasInstance(this))
	{
		UGameplayMessageSubsystem& GMS = UGameplayMessageSubsystem::Get(this);
		GMS.BroadcastMessage(SFGameplayTags::Message_Game_GameOverStats, GameOverResult);
	}
}

void USFGameOverManagerComponent::OnRep_ReadyCount()
{
	if (UGameplayMessageSubsystem::HasInstance(this))
	{
		UGameplayMessageSubsystem& GMS = UGameplayMessageSubsystem::Get(this);
		FSFLobbyReadyMessage Message;
		Message.ReadyCount = ReadyCount;
		Message.TotalCount = GetTotalPlayerCount();
		GMS.BroadcastMessage(SFGameplayTags::Message_Game_LobbyReadyCount, Message);
	}
}
