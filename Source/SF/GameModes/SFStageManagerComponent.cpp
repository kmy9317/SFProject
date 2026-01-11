#include "SFStageManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Animation/Hero/AnimNotify/SFAnimNotify_SendGameplayEvent.h"
#include "Net/UnrealNetwork.h"
#include "Player/SFPlayerState.h"
#include "System/SFPlayFabSubsystem.h"
#include "System/SFStageSubsystem.h"

USFStageManagerComponent::USFStageManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void USFStageManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, CurrentStageInfo);
    DOREPLIFETIME(ThisClass, bStageCleared);
    DOREPLIFETIME(ThisClass, CurrentBossActor);
}

void USFStageManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    // 서버에서만 Subsystem에서 현재 스테이지 정보 가져오기
    if (GetOwner()->HasAuthority())
    {
        if (UGameInstance* GI = GetWorld()->GetGameInstance())
        {
            if (USFStageSubsystem* StageSubsystem = GI->GetSubsystem<USFStageSubsystem>())
            {
                CurrentStageInfo = StageSubsystem->GetCurrentStageInfo();
            }
        }
    }
}

void USFStageManagerComponent::NotifyStageClear()
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    if (bStageCleared)
    {
        return;
    }

    // ===============================
    // 🔥 GAS StageClear 이벤트 전달
    // ===============================
    if (UWorld* World = GetWorld())
    {
        if (AGameStateBase* GameState = World->GetGameState())
        {
            for (APlayerState* BasePS : GameState->PlayerArray)
            {
                ASFPlayerState* PS = Cast<ASFPlayerState>(BasePS);
                if (!PS)
                    continue;

                UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
                if (!ASC)
                    continue;

                FGameplayEventData EventData;
                EventData.EventTag = SFGameplayTags::GameplayEvent_Stage_Clear;

                ASC->HandleGameplayEvent(
                    EventData.EventTag,
                    &EventData
                );
            }
        }
    }
    
    SaveLocalPlayerGoldToPlayFab();
    bStageCleared = true;

    OnStageCleared.Broadcast(CurrentStageInfo);
}

void USFStageManagerComponent::OnRep_bStageCleared()
{
    if (bStageCleared)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameOverManager] Stage Cleared!"));
        SaveLocalPlayerGoldToPlayFab();
        OnStageCleared.Broadcast(CurrentStageInfo);
    }
}


//-------------------------------------------------------------------------

void USFStageManagerComponent::RegisterBossActor(ACharacter* NewBoss)
{
    if (!GetOwner()->HasAuthority()) return;
    CurrentBossActor = NewBoss;
    OnBossStateChanged.Broadcast(CurrentBossActor);    
}

int32 USFStageManagerComponent::GetPlayerCount() const
{
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (USFStageSubsystem* StageSubsystem = GI->GetSubsystem<USFStageSubsystem>())
            {
                return StageSubsystem->GetPlayerCount();
            }
        }
    }
    return 1;
}

FSFEnemyScalingContext USFStageManagerComponent::GetEnemyScalingContext() const
{
    FSFEnemyScalingContext Context;

    if (UGameInstance* GI = GetWorld()->GetGameInstance())
    {
        if (USFStageSubsystem* StageSubsystem = GI->GetSubsystem<USFStageSubsystem>())
        {
            const FSFStageInfo& StageInfo = StageSubsystem->GetCurrentStageInfo();
            Context.StageIndex = StageInfo.StageIndex;
            Context.SubStageIndex = StageInfo.SubStageIndex;
            Context.PlayerCount = StageSubsystem->GetPlayerCount();
            Context.bIsBossStage = StageInfo.IsBossStage();
            Context.bIsFinalStage = StageInfo.bIsFinalStage;
        }
    }
    
    return Context;
}

void USFStageManagerComponent::OnRep_CurrentBossActor()
{
    OnBossStateChanged.Broadcast(CurrentBossActor);
}

void USFStageManagerComponent::SaveLocalPlayerGoldToPlayFab()
{
    // 1. 로컬 플레이어 컨트롤러 가져오기
    APlayerController* LocalPC = GetWorld()->GetFirstPlayerController();
    if (!LocalPC || !LocalPC->IsLocalController())
    {
        UE_LOG(LogTemp, Error, TEXT("[GameOverManager] LocalControllerFailed"));
        return;
    }

    // 2. PlayerState 가져오기
    ASFPlayerState* PS = LocalPC->GetPlayerState<ASFPlayerState>();
    if (!PS)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameOverManager] PSFailed"));
        return;
    }

    // 3. PlayFab Subsystem 가져오기
    UGameInstance* GI = GetWorld()->GetGameInstance();
    if (USFPlayFabSubsystem* PlayFabSubsystem = GI ? GI->GetSubsystem<USFPlayFabSubsystem>() : nullptr)
    {
        int32 CurrentGold = PS->GetGold();

        UE_LOG(LogTemp, Error, TEXT("[GameOverManager] Saving Gold to PlayFab: %d"), CurrentGold);

        // 4. 데이터 갱신 및 저장 요청
        PlayFabSubsystem->SetGold(CurrentGold);
        PlayFabSubsystem->SavePlayerData();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[GameOverManager] PFSFailed"));
    }
}


