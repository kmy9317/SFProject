#include "SFGA_Interact_RewardChest.h"

#include "Abilities/Tasks/AbilityTask_NetworkSyncPoint.h"
#include "Actors/SFRewardChest.h"
#include "Player/SFPlayerController.h"
#include "Player/SFPlayerState.h"
#include "Player/Components/SFCommonUpgradeComponent.h"
#include "System/Data/SFGameData.h"
#include "UI/Common/SFSkillSelectionScreen.h"
#include "UI/Common/SFStatBoostSelectionWidget.h"
#include "System/Data/Common/SFCommonLootTable.h"

USFGA_Interact_RewardChest::USFGA_Interact_RewardChest(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USFGA_Interact_RewardChest::OnChestOpened(ASFChestBase* ChestActor)
{
	ASFRewardChest* RewardChest = Cast<ASFRewardChest>(ChestActor);
	if (!RewardChest)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	CachedRewardChest = RewardChest;

	switch (RewardChest->GetRewardType())
	{
	case ESFRewardChestType::SkillUpgrade:
		if (IsLocallyControlled())
		{
			ShowSkillUpgradeUI(RewardChest->GetStageIndex());
		}
		break;

	case ESFRewardChestType::StatBoost:
		InitializeStatBoostSelection(RewardChest->GetStageIndex());
		break;
	default:
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		break;
	}
}

void USFGA_Interact_RewardChest::ShowSkillUpgradeUI(int32 StageIndex)
{
	ASFPlayerController* PC = GetSFPlayerControllerFromActorInfo();
	if (!PC)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	if (!SkillSelectionScreenClass)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	SkillSelectionScreen = CreateWidget<USFSkillSelectionScreen>(PC, SkillSelectionScreenClass);
	if (!SkillSelectionScreen)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 선택 완료 델리게이트 바인딩
	SkillSelectionScreen->OnSelectionCompleteDelegate.AddDynamic(this, &ThisClass::OnSkillSelectionComplete);

	// 스테이지 정보로 초기화
	SkillSelectionScreen->InitializeSelection(StageIndex);
	SkillSelectionScreen->AddToViewport(50);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(SkillSelectionScreen->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;
}

void USFGA_Interact_RewardChest::OnSkillSelectionComplete()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Interact_RewardChest::InitializeStatBoostSelection(int32 StageIndex)
{
	ASFPlayerState* PS = GetSFPlayerStateFromActorInfo();
	if (!PS)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	USFCommonUpgradeComponent* UpgradeComp = PS->FindComponentByClass<USFCommonUpgradeComponent>();
	if (!UpgradeComp)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 로컬: 델리게이트 바인딩
	if (IsLocallyControlled() && !bStatBoostDelegatesBound)
	{
		UpgradeComp->OnChoicesReceived.AddDynamic(this, &ThisClass::OnStatBoostChoicesReceived);
		UpgradeComp->OnUpgradeApplied.AddDynamic(this, &ThisClass::OnStatBoostApplied);
		UpgradeComp->OnUpgradeApplyFailed.AddDynamic(this, &ThisClass::OnStatBoostApplyFailed);
		UpgradeComp->OnRerollFailed.AddDynamic(this, &ThisClass::OnRerollFailed);
		bStatBoostDelegatesBound = true;
	}

	CachedStageIndex = StageIndex;

	if (UAbilityTask_NetworkSyncPoint* WaitTask = UAbilityTask_NetworkSyncPoint::WaitNetSync(this, EAbilityTaskNetSyncType::OnlyServerWait))
	{
		WaitTask->OnSync.AddDynamic(this, &ThisClass::OnClientReadyForStatBoost);
		WaitTask->ReadyForActivation();
	}
}

void USFGA_Interact_RewardChest::OnClientReadyForStatBoost()
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	ASFPlayerState* PS = GetSFPlayerStateFromActorInfo();
	if (!PS)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	USFCommonUpgradeComponent* UpgradeComp = PS->FindComponentByClass<USFCommonUpgradeComponent>();
	if (!UpgradeComp)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	const USFGameData& GameData = USFGameData::Get();
	USFCommonLootTable* LootTable = GameData.DefaultCommonLootTable.LoadSynchronous();
	if (!LootTable)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UpgradeComp->RequestGenerateChoices(LootTable, CachedStageIndex, 3);
}

void USFGA_Interact_RewardChest::OnStatBoostChoicesReceived(const TArray<FSFCommonUpgradeChoice>& Choices, int32 NextRerollCost)
{
    if (Choices.IsEmpty())
    {
        CleanupStatBoostDelegates();
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

	if (!StatBoostWidget)
	{
		ShowStatBoostUI(Choices, NextRerollCost);
		return;
	}

	// 추가 선택 보너스 여부 확인
	ASFPlayerState* PS = GetSFPlayerStateFromActorInfo();
	USFCommonUpgradeComponent* UpgradeComp = PS ? PS->FindComponentByClass<USFCommonUpgradeComponent>() : nullptr;
	if (UpgradeComp && UpgradeComp->IsPendingExtraSelection())
	{
		StatBoostWidget->RefreshChoicesWithExtraNotice(Choices, NextRerollCost);
	}
	else
	{
		StatBoostWidget->RefreshChoices(Choices, NextRerollCost);
	}
}

void USFGA_Interact_RewardChest::ShowStatBoostUI(const TArray<FSFCommonUpgradeChoice>& Choices, int32 NextRerollCost)
{
    ASFPlayerController* PC = GetSFPlayerControllerFromActorInfo();
    if (!PC)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    // 이미 위젯이 있으면 갱신만 (리롤/추가 선택 시)
    if (StatBoostWidget)
    {
        StatBoostWidget->RefreshChoices(Choices, NextRerollCost);
        return;
    }

    // 위젯 클래스 체크
    if (!StatBoostWidgetClass)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    // 새 위젯 생성
    StatBoostWidget = CreateWidget<USFStatBoostSelectionWidget>(PC, StatBoostWidgetClass);
    if (!StatBoostWidget)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    StatBoostWidget->OnCardSelectedDelegate.AddDynamic(this, &ThisClass::OnStatBoostCardSelected);
	StatBoostWidget->OnSelectionCompleteDelegate.AddDynamic(this, &ThisClass::OnStatBoostSelectionComplete);
    StatBoostWidget->InitializeWithChoices(Choices, NextRerollCost);
    StatBoostWidget->AddToViewport(50);

    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(StatBoostWidget->TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PC->SetInputMode(InputMode);
    PC->bShowMouseCursor = true;
}

void USFGA_Interact_RewardChest::OnStatBoostCardSelected(int32 ChoiceIndex)
{
    ASFPlayerState* PS = GetSFPlayerStateFromActorInfo();
    if (!PS)
    {
        return;
    }

    USFCommonUpgradeComponent* UpgradeComp = PS->FindComponentByClass<USFCommonUpgradeComponent>();
    if (UpgradeComp)
    {
        UpgradeComp->RequestApplyUpgradeByIndex(ChoiceIndex);
    }
}

void USFGA_Interact_RewardChest::OnStatBoostSelectionComplete()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Interact_RewardChest::OnStatBoostApplied()
{
	if (StatBoostWidget)
	{
		StatBoostWidget->NotifyClose();
	}
	else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void USFGA_Interact_RewardChest::OnStatBoostApplyFailed(const FText& Reason)
{
    // UI에 실패 메시지 표시 
    if (StatBoostWidget)
    {
        StatBoostWidget->ShowErrorMessage(Reason);
    }
}

void USFGA_Interact_RewardChest::OnRerollFailed(const FText& Reason)
{
    // UI에 리롤 실패 메시지 표시
    if (StatBoostWidget)
    {
        StatBoostWidget->ShowErrorMessage(Reason);
    }
}

void USFGA_Interact_RewardChest::CleanupUI()
{
	if (SkillSelectionScreen)
	{
		SkillSelectionScreen->OnSelectionCompleteDelegate.RemoveAll(this);
		SkillSelectionScreen->RemoveFromParent();
		SkillSelectionScreen = nullptr;
	}

	if (StatBoostWidget)
	{
		StatBoostWidget->OnCardSelectedDelegate.RemoveAll(this);
		StatBoostWidget->RemoveFromParent();
		StatBoostWidget = nullptr;
	}
	
	if (ASFPlayerController* PC = GetSFPlayerControllerFromActorInfo())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}

void USFGA_Interact_RewardChest::CleanupStatBoostDelegates()
{
	if (!bStatBoostDelegatesBound)
	{
		return;
	}

	ASFPlayerState* PS = GetSFPlayerStateFromActorInfo();
	if (!PS)
	{
		return;
	}

	USFCommonUpgradeComponent* UpgradeComp = PS->FindComponentByClass<USFCommonUpgradeComponent>();
	if (UpgradeComp)
	{
		UpgradeComp->OnChoicesReceived.RemoveAll(this);
		UpgradeComp->OnUpgradeApplied.RemoveAll(this);
		UpgradeComp->OnUpgradeApplyFailed.RemoveAll(this);
		UpgradeComp->OnRerollFailed.RemoveAll(this);
	}

	bStatBoostDelegatesBound = false;
}

void USFGA_Interact_RewardChest::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsLocallyControlled())
	{
		CleanupUI();
		CleanupStatBoostDelegates();
	}

	CachedRewardChest.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
