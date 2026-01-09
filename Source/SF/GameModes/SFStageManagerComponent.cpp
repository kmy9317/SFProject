#include "SFStageManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayEvent/SFGameplayEventTags.h"
#include "Animation/Hero/AnimNotify/SFAnimNotify_SendGameplayEvent.h"
#include "Net/UnrealNetwork.h"
#include "Player/SFPlayerState.h"
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
    
    bStageCleared = true;

    OnStageCleared.Broadcast(CurrentStageInfo);
}

void USFStageManagerComponent::OnRep_bStageCleared()
{
    if (bStageCleared)
    {
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


void USFStageManagerComponent::OnRep_CurrentBossActor()
{
    OnBossStateChanged.Broadcast(CurrentBossActor);
}




