#include "SFGameOverManagerComponent.h"

#include "Messages/SFMessageGameplayTags.h"
#include "Messages/SFPortalInfoMessages.h"
#include "Net/UnrealNetwork.h"
#include "Player/Components/SFPlayerCombatStateComponent.h"

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
	OnRep_bGameOver();
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
