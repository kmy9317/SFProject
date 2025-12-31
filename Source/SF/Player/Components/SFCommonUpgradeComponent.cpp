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

void USFCommonUpgradeComponent::RequestGenerateChoices(USFCommonLootTable* LootTable, int32 StageIndex, int32 Count)
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
	TArray<FSFCommonUpgradeChoice> GeneratedChoices = Subsystem->GenerateUpgradeOptions(PS, LootTable, Count);

	// 클라이언트로 전송
	Client_ReceiveUpgradeChoices(GeneratedChoices, false);
}

void USFCommonUpgradeComponent::Client_ReceiveUpgradeChoices_Implementation(const TArray<FSFCommonUpgradeChoice>& Choices, bool bIsExtraSelection)
{
	PendingChoices = Choices;
	bPendingExtraSelection = bIsExtraSelection;
	
	// UI에 알림
	OnChoicesReceived.Broadcast(Choices);

	UE_LOG(LogSF, Warning, TEXT("Client_ReceiveUpgradeChoices: Received %d choices"), Choices.Num());
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
	bool bSuccess = Subsystem->ApplyUpgradeChoice(PS, ChoiceId);
	if (!bSuccess)
	{
		Client_NotifyUpgradeApplyFailed(NSLOCTEXT("SF", "ApplyFailed_InvalidChoice", "유효하지 않은 선택입니다."));
		return;
	}

	// 추가 선택 태그 체크
	if (HasExtraSelectionTag())
	{
		// 태그 소모
		ConsumeExtraSelectionTag();

		// 새로운 선택지 생성
		TArray<FSFCommonUpgradeChoice> NewChoices = Subsystem->GenerateUpgradeOptions(PS, CachedLootTable, CachedChoiceCount);

		// 추가 선택임을 알리며 전송
		Client_ReceiveUpgradeChoices(NewChoices, true);
		return;
	}

	// 적용 완료 알림
	Client_NotifyUpgradeApplied();
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
		Client_NotifyRerollFailed(NSLOCTEXT("SF", "RerollFailed_InvalidWorld", "잘못된 요청입니다."));
		return;
	}

	USFCommonUpgradeManagerSubsystem* Subsystem = World->GetSubsystem<USFCommonUpgradeManagerSubsystem>();
	if (!Subsystem)
	{
		Client_NotifyRerollFailed(NSLOCTEXT("SF", "RerollFailed_NoSubsystem", "시스템 오류입니다."));
		return;
	}

	// 리롤 → 새 선택지 반환
	TArray<FSFCommonUpgradeChoice> NewChoices = Subsystem->TryRerollOptions(PS);
	if (NewChoices.IsEmpty())
	{
		Client_NotifyRerollFailed(NSLOCTEXT("SF", "RerollFailed_NoTicket", "리롤 티켓이 부족합니다."));
		return;
	}

	Client_ReceiveUpgradeChoices(NewChoices, false);
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

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	// TODO: 리롤 티켓 태그 정의 후 수정
	// return ASC->GetGameplayTagCount(TAG_RerollTicket) > 0;
	return true;
}

bool USFCommonUpgradeComponent::HasExtraSelectionTag() const
{
	ASFPlayerState* PS = GetOwner<ASFPlayerState>();
	if (!PS)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	// TODO: 추가 선택 태그 정의 후 수정
	// return ASC->HasMatchingGameplayTag(TAG_ExtraSelection);
	return false;
}

void USFCommonUpgradeComponent::ConsumeExtraSelectionTag()
{
	ASFPlayerState* PS = GetOwner<ASFPlayerState>();
	if (!PS)
	{
		return;
	}

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// TODO: 추가 선택 태그 정의 후 수정
	// ASC->RemoveLooseGameplayTag(TAG_ExtraSelection);
}

void USFCommonUpgradeComponent::ClearPendingChoices()
{
	PendingChoices.Empty();
}