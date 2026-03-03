#include "SFCommonUpgradeComponent.h"

#include "SFLogChannels.h"
#include "Player/SFPlayerState.h"
#include "System/Data/Common/SFCommonUpgradeChoice.h"
#include "System/Data/Common/SFCommonUpgradeManagerSubsystem.h"


USFCommonUpgradeComponent::USFCommonUpgradeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

FGuid USFCommonUpgradeComponent::RequestGenerateChoices(USFCommonLootTable* LootTable, int32 StageIndex, int32 Count, FOnUpgradeComplete OnComplete, AActor* SourceInteractable)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return FGuid();
	}

	ASFPlayerState* PS = GetOwner<ASFPlayerState>();
	if (!PS)
	{
		return FGuid();
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return FGuid();
	}

	USFCommonUpgradeManagerSubsystem* Subsystem = World->GetSubsystem<USFCommonUpgradeManagerSubsystem>();
	if (!Subsystem)
	{
		return FGuid();
	}

	CachedLootTable = LootTable;
	CachedStageIndex = StageIndex;
	CachedChoiceCount = Count;

	TArray<FSFCommonUpgradeChoice> GeneratedChoices;
	FGuid ContextId = Subsystem->GenerateUpgradeOptions(PS, LootTable, Count, MoveTemp(OnComplete), SourceInteractable, GeneratedChoices);

	if (!ContextId.IsValid())
	{
		return FGuid();
	}

	int32 NextCost = Subsystem->CalculateRerollCost(ContextId);
	Client_ReceiveUpgradeChoices(ContextId, GeneratedChoices, false, NextCost);

	return ContextId;
}

void USFCommonUpgradeComponent::Client_ReceiveUpgradeChoices_Implementation(const FGuid& ContextId, const TArray<FSFCommonUpgradeChoice>& Choices, bool bIsExtraSelection, int32 NextRerollCost)
{
	PendingChoicesMap.FindOrAdd(ContextId).Choices = Choices;
	CachedNextRerollCost = NextRerollCost;
	bPendingExtraSelection = bIsExtraSelection;
	OnChoicesReceived.Broadcast(ContextId, Choices, NextRerollCost);
}

void USFCommonUpgradeComponent::RequestApplyUpgrade(const FGuid& ContextId, const FGuid& ChoiceId)
{
	if (!ChoiceId.IsValid())
	{
		return;
	}

	Server_RequestApplyUpgrade(ContextId, ChoiceId);
}

void USFCommonUpgradeComponent::RequestApplyUpgradeByIndex(const FGuid& ContextId, int32 ChoiceIndex)
{
	FSFPendingUpgradeData* Data = PendingChoicesMap.Find(ContextId);
	if (!Data || !Data->Choices.IsValidIndex(ChoiceIndex))
	{
		return;
	}
	Server_RequestApplyUpgrade(ContextId, Data->Choices[ChoiceIndex].UniqueId);
}

void USFCommonUpgradeComponent::Server_RequestApplyUpgrade_Implementation(const FGuid& ContextId, const FGuid& ChoiceId)
{
	ASFPlayerState* PS = GetOwner<ASFPlayerState>();
	if (!PS)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	USFCommonUpgradeManagerSubsystem* Subsystem = World->GetSubsystem<USFCommonUpgradeManagerSubsystem>();
	if (!Subsystem)
	{
		return;
	}

	// 서브시스템에서 검증 및 적용
	ESFUpgradeApplyResult Result = Subsystem->ApplyUpgradeChoice(ContextId, ChoiceId);
	switch (Result)
	{
	case ESFUpgradeApplyResult::Failed:
		Client_NotifyUpgradeApplyFailed(NSLOCTEXT("SF", "ApplyFailed_InvalidChoice", "유효하지 않은 선택입니다."));
		break;

	case ESFUpgradeApplyResult::MoreEnhance:
		{
			TArray<FSFCommonUpgradeChoice> NewChoices = Subsystem->RegenerateChoicesForMoreEnhance(ContextId);
			int32 NextCost = Subsystem->CalculateRerollCost(ContextId);
			Client_ReceiveUpgradeChoices(ContextId, NewChoices, true, NextCost);
		}
		break;

	case ESFUpgradeApplyResult::Success:
		Client_NotifyUpgradeApplied();
		break;
	}
}

void USFCommonUpgradeComponent::Client_NotifyUpgradeApplied_Implementation()
{
	OnUpgradeApplied.Broadcast();
}

void USFCommonUpgradeComponent::Client_NotifyUpgradeApplyFailed_Implementation(const FText& Reason)
{
	OnUpgradeApplyFailed.Broadcast(Reason);
}

void USFCommonUpgradeComponent::RequestReroll(const FGuid& ContextId)
{
	if (!CanReroll(ContextId))
	{
		return;
	}
	Server_RequestReroll(ContextId);
}

void USFCommonUpgradeComponent::Server_RequestReroll_Implementation(const FGuid& ContextId)
{
	ASFPlayerState* PS = GetOwner<ASFPlayerState>();
	if (!PS)
	{
		Client_NotifyRerollFailed(NSLOCTEXT("SF", "RerollFailed_InvalidState", "잘못된 요청입니다."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	USFCommonUpgradeManagerSubsystem* Subsystem = World->GetSubsystem<USFCommonUpgradeManagerSubsystem>();
	if (!Subsystem)
	{
		Client_NotifyRerollFailed(NSLOCTEXT("SF", "RerollFailed_NoSubsystem", "시스템 오류입니다."));
		return;
	}
	
	// 리롤 가능 여부 체크
	if (!Subsystem->CanReroll(ContextId))
	{
		Client_NotifyRerollFailed(NSLOCTEXT("SF", "RerollFailed_NotEnoughGold", "골드가 부족합니다."));
		return;
	}

	// 리롤 → 새 선택지 반환
	TArray<FSFCommonUpgradeChoice> NewChoices = Subsystem->TryRerollOptions(ContextId);
	if (NewChoices.IsEmpty())
	{
		Client_NotifyRerollFailed(NSLOCTEXT("SF", "RerollFailed_NoTicket", "리롤 티켓이 부족합니다."));
		return;
	}

	int32 NextCost = Subsystem->CalculateRerollCost(ContextId);
	Client_ReceiveUpgradeChoices(ContextId, NewChoices, false, NextCost);
}

void USFCommonUpgradeComponent::Client_NotifyRerollFailed_Implementation(const FText& Reason)
{
	OnRerollFailed.Broadcast(Reason);
}

bool USFCommonUpgradeComponent::CanReroll(const FGuid& ContextId) const
{
	const FSFPendingUpgradeData* Data = PendingChoicesMap.Find(ContextId);
	if (!Data || Data->Choices.IsEmpty())
	{
		return false;
	}

	ASFPlayerState* PS = GetOwner<ASFPlayerState>();
	if (!PS)
	{
		return false;
	}

	return (CachedNextRerollCost == 0) || (PS->GetGold() >= CachedNextRerollCost);
}

void USFCommonUpgradeComponent::ClearPendingChoices(const FGuid& ContextId)
{
	PendingChoicesMap.Remove(ContextId);
}