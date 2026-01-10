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

void USFCommonUpgradeComponent::RequestGenerateChoices(USFCommonLootTable* LootTable, int32 StageIndex, int32 Count, FOnUpgradeComplete OnComplete, AActor* SourceInteractable)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

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
	
	CachedLootTable = LootTable;
	CachedStageIndex = StageIndex;
	CachedChoiceCount = Count;
	
	// 서브시스템에서 선택지 생성
	TArray<FSFCommonUpgradeChoice> GeneratedChoices = Subsystem->GenerateUpgradeOptions(PS, LootTable, Count, MoveTemp(OnComplete), SourceInteractable);


	// 다음 리롤 비용 계산
	int32 NextCost = Subsystem->CalculateRerollCost(PS);

	// 클라이언트로 전송
	Client_ReceiveUpgradeChoices(GeneratedChoices, false, NextCost);
}

void USFCommonUpgradeComponent::Client_ReceiveUpgradeChoices_Implementation(const TArray<FSFCommonUpgradeChoice>& Choices, bool bIsExtraSelection, int32 NextRerollCost)
{
	PendingChoices = Choices;
	CachedNextRerollCost = NextRerollCost;
	bPendingExtraSelection = bIsExtraSelection;
	OnChoicesReceived.Broadcast(Choices, NextRerollCost);
}

void USFCommonUpgradeComponent::RequestApplyUpgrade(const FGuid& ChoiceId)
{
	if (!ChoiceId.IsValid())
	{
		return;
	}

	Server_RequestApplyUpgrade(ChoiceId);
}

void USFCommonUpgradeComponent::RequestApplyUpgradeByIndex(int32 ChoiceIndex)
{
	if (!PendingChoices.IsValidIndex(ChoiceIndex))
	{
		return;
	}

	Server_RequestApplyUpgrade(PendingChoices[ChoiceIndex].UniqueId);
}

void USFCommonUpgradeComponent::Server_RequestApplyUpgrade_Implementation(const FGuid& ChoiceId)
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
	ESFUpgradeApplyResult Result = Subsystem->ApplyUpgradeChoice(PS, ChoiceId);
	switch (Result)
	{
	case ESFUpgradeApplyResult::Failed:
		Client_NotifyUpgradeApplyFailed(NSLOCTEXT("SF", "ApplyFailed_InvalidChoice", "유효하지 않은 선택입니다."));
		break;

	case ESFUpgradeApplyResult::MoreEnhance:
		{
			TArray<FSFCommonUpgradeChoice> NewChoices = Subsystem->RegenerateChoicesForMoreEnhance(PS);
			int32 NextCost = Subsystem->CalculateRerollCost(PS);
			Client_ReceiveUpgradeChoices(NewChoices, true, NextCost);
		}
		break;

	case ESFUpgradeApplyResult::Success:
		Client_NotifyUpgradeApplied();
		break;
	}
}

void USFCommonUpgradeComponent::Client_NotifyUpgradeApplied_Implementation()
{
	PendingChoices.Empty();
	OnUpgradeApplied.Broadcast();
}

void USFCommonUpgradeComponent::Client_NotifyUpgradeApplyFailed_Implementation(const FText& Reason)
{
	PendingChoices.Empty();
	OnUpgradeApplyFailed.Broadcast(Reason);
}

void USFCommonUpgradeComponent::RequestReroll()
{
	if (!CanReroll())
	{
		return;
	}

	Server_RequestReroll();
}

void USFCommonUpgradeComponent::Server_RequestReroll_Implementation()
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
	if (!Subsystem->CanReroll(PS))
	{
		Client_NotifyRerollFailed(NSLOCTEXT("SF", "RerollFailed_NotEnoughGold", "골드가 부족합니다."));
		return;
	}

	// 리롤 → 새 선택지 반환
	TArray<FSFCommonUpgradeChoice> NewChoices = Subsystem->TryRerollOptions(PS);
	if (NewChoices.IsEmpty())
	{
		Client_NotifyRerollFailed(NSLOCTEXT("SF", "RerollFailed_NoTicket", "리롤 티켓이 부족합니다."));
		return;
	}

	int32 NextCost = Subsystem->CalculateRerollCost(PS);

	Client_ReceiveUpgradeChoices(NewChoices, false, NextCost);
}

void USFCommonUpgradeComponent::Client_NotifyRerollFailed_Implementation(const FText& Reason)
{
	OnRerollFailed.Broadcast(Reason);
}

bool USFCommonUpgradeComponent::CanReroll() const
{
	if (PendingChoices.IsEmpty())
	{
		return false;
	}

	// 리롤 티켓 확인 (ASC에서 태그 카운트)
	ASFPlayerState* PS = GetOwner<ASFPlayerState>();
	if (!PS)
	{
		return false;
	}

	// 무료이거나 골드 충분
	return (CachedNextRerollCost == 0) || (PS->GetGold() >= CachedNextRerollCost);
}

void USFCommonUpgradeComponent::ClearPendingChoices()
{
	PendingChoices.Empty();
}