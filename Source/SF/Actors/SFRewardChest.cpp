#include "SFRewardChest.h"

#include "GameModes/SFGameState.h"
#include "GameModes/SFStageManagerComponent.h"
#include "Net/UnrealNetwork.h"

ASFRewardChest::ASFRewardChest(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	bShouldConsume = false;
}

void ASFRewardChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, RewardType);
	DOREPLIFETIME(ThisClass, bIsActivated);
	DOREPLIFETIME(ThisClass, CachedStageIndex);
	DOREPLIFETIME(ThisClass, ClaimedPlayers);
}

void ASFRewardChest::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	// 에디터 오버라이드 체크 (테스트용 보상 타입 강제)
	if (bOverrideRewardType)
	{
		RewardType = OverriddenRewardType;
	}

	// 스테이지 클리어 이벤트 바인딩
	if (ASFGameState* SFGameState = GetWorld()->GetGameState<ASFGameState>())
	{
		if (USFStageManagerComponent* StageManager = SFGameState->GetStageManager())
		{
			if (StageManager->IsStageCleared())
			{
				OnStageClearedHandler(StageManager->GetCurrentStageInfo());
			}
			else
			{
				StageManager->OnStageCleared.AddDynamic(this, &ThisClass::OnStageClearedHandler);
			}
		}
	}
}

bool ASFRewardChest::CanInteraction(const FSFInteractionQuery& InteractionQuery) const
{
	if (!bIsActivated || RewardType == ESFRewardChestType::None)
	{
		return false;
	}

	if (AController* Controller = InteractionQuery.RequestingController.Get())
	{
		if (HasPlayerClaimed(Controller->PlayerState))
		{
			return false;
		}
	}

	return Super::CanInteraction(InteractionQuery);
}

void ASFRewardChest::OnInteractionSuccess(AActor* Interactor)
{
	if (HasAuthority())
	{
		APawn* Pawn = Cast<APawn>(Interactor);
		APlayerState* PS = Pawn ? Pawn->GetPlayerState() : nullptr;
        
		if (PS && !ClaimedPlayers.Contains(PS))
		{
			ClaimedPlayers.Add(PS);
		}
	}
	
	Super::OnInteractionSuccess(Interactor);
}

bool ASFRewardChest::HasPlayerClaimed(APlayerState* PlayerState) const
{
	if (!PlayerState)
	{
		return false;
	}

	return ClaimedPlayers.Contains(PlayerState);
}

void ASFRewardChest::OnStageClearedHandler(const FSFStageInfo& ClearedStageInfo)
{
	if (!HasAuthority())
	{
		return;
	}

	CachedStageIndex = ClearedStageInfo.StageIndex;
	if (!bOverrideRewardType)
	{
		RewardType = DetermineRewardType(ClearedStageInfo);
	}
	bIsActivated = true;

	OnRep_RewardType();
	OnRep_IsActivated();
}

ESFRewardChestType ASFRewardChest::DetermineRewardType(const FSFStageInfo& StageInfo) const
{
	if (StageInfo.IsBossStage())
	{
		return ESFRewardChestType::SkillUpgrade;
	}

	// TODO : 추가적인 스테이지 타입 보상 기능 추가 가능.
	return ESFRewardChestType::StatBoost;
}

void ASFRewardChest::OnRep_RewardType()
{
	// TODO : 클라이언트에서 보상 타입 변경 시 비주얼 업데이트 가능 
}

void ASFRewardChest::OnRep_IsActivated()
{
	// TODO : 활성화 시 비주얼 이펙트 표기 관련
}


