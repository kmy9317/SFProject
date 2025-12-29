#include "SFGA_Interact_RewardChest.h"

#include "Actors/SFRewardChest.h"
#include "Player/SFPlayerController.h"
#include "UI/Common/SFSkillSelectionScreen.h"

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

	if (IsLocallyControlled())
	{
		switch (RewardChest->GetRewardType())
		{
		case ESFRewardChestType::SkillUpgrade:
			ShowSkillUpgradeUI(RewardChest->GetStageIndex());
			return;
		case ESFRewardChestType::StatBoost:
			ShowStatBoostUI(RewardChest->GetStageIndex());
			return;
		default:
			break;
		}
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

void USFGA_Interact_RewardChest::ShowStatBoostUI(int32 StageIndex)
{
	// TODO: 일반 강화 UI 구현 예정
	// StatBoostWidgetClass 사용하여 위젯 생성
	// OnStatBoostSelectionComplete 바인딩
	// 특정 선택지 한번 더 뽑을수 있는 Tag가 부여된 플레이어 한테는 Widget 종료하지 않고 한번더 Refresh 해서 선택할 수 있도록 처리
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Interact_RewardChest::OnSkillSelectionComplete()
{
	CleanupUI();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGA_Interact_RewardChest::OnStatBoostSelectionComplete()
{
	CleanupUI();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
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
		StatBoostWidget->RemoveFromParent();
		StatBoostWidget = nullptr;
	}
	
	if (ASFPlayerController* PC = GetSFPlayerControllerFromActorInfo())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}

void USFGA_Interact_RewardChest::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsLocallyControlled())
	{
		CleanupUI();
	}

	CachedRewardChest.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}