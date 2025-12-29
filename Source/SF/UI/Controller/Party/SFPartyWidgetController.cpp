// Fill out your copyright notice in the Description page of Project Settings.


#include "SFPartyWidgetController.h"

#include "SFLogChannels.h"
#include "SFPartyMemberWidgetController.h"
#include "GameModes/SFGameState.h"
#include "Player/SFPlayerState.h"

void USFPartyWidgetController::BindCallbacksToDependencies()
{
	ASFGameState* SFGameState = Cast<ASFGameState>(GameState);
	if (!SFGameState)
	{
		UE_LOG(LogSF, Warning, TEXT("USFPartyWidgetController: GameState is not ASFGameState."));
		return;
	}

	SFGameState->OnPlayerAdded.AddDynamic(this, &USFPartyWidgetController::HandlePlayerAdded);
	SFGameState->OnPlayerRemoved.AddDynamic(this, &USFPartyWidgetController::HandlePlayerRemoved);
}

void USFPartyWidgetController::BroadcastInitialSets()
{
	ASFGameState* SFGameState = Cast<ASFGameState>(GameState);
	if (!SFGameState)
	{
		return;
	}

	// 초기 플레이어들 추가 (정렬은 GetSortedMemberControllers에서 처리)
	for (APlayerState* PS : SFGameState->PlayerArray)
	{
		HandlePlayerAdded(PS);
	}
}

TArray<USFPartyMemberWidgetController*> USFPartyWidgetController::GetSortedMemberControllers() const
{
	TArray<USFPartyMemberWidgetController*> Result;
    
	for (const auto& Pair : PartyMemberControllerMap)
	{
		if (Pair.Value)
		{
			Result.Add(Pair.Value);
		}
	}

	// PlayerSlot 기준 정렬
	Result.Sort([](const USFPartyMemberWidgetController& A, const USFPartyMemberWidgetController& B)
	{
		const ASFPlayerState* PSA = Cast<ASFPlayerState>(A.GetTargetPlayerState());
		const ASFPlayerState* PSB = Cast<ASFPlayerState>(B.GetTargetPlayerState());
		if (PSA && PSB)
		{
			return PSA->GetPlayerSelection().GetPlayerSlot() < PSB->GetPlayerSelection().GetPlayerSlot();
		}
		return false;
	});

	return Result;
}

void USFPartyWidgetController::HandlePlayerAdded(APlayerState* InPlayerState)
{
	// 로컬 플레이어는 파티 목록에 추가하지 않음
	if (InPlayerState == TargetPlayerState)
	{
		return;
	}

	ASFPlayerState* SFInPlayerState = Cast<ASFPlayerState>(InPlayerState);
	if (SFInPlayerState)
	{
		AddAndBroadcastMember(SFInPlayerState);
	}
}

void USFPartyWidgetController::HandlePlayerRemoved(APlayerState* InPlayerState)
{
	ASFPlayerState* SFInPlayerState = Cast<ASFPlayerState>(InPlayerState);
	if (SFInPlayerState)
	{
		RemoveAndBroadcastMember(SFInPlayerState);
	}
}

void USFPartyWidgetController::HandlePlayerInfoChangedForReorder(const FSFPlayerSelectionInfo& NewPlayerSelection)
{
	OnPartyOrderChanged.Broadcast();
}

void USFPartyWidgetController::AddAndBroadcastMember(ASFPlayerState* PlayerState)
{
	// TMap에 이미 존재하는지 확인 
	if (PartyMemberControllerMap.Contains(PlayerState))
	{
		UE_LOG(LogSF, Warning, TEXT("USFPartyWidgetController: PlayerState is already in PartyMemberControllerMap."));
		return;
	}

	// 다른 플레이어를 위한 새 (WidgetController) 생성
	USFPartyMemberWidgetController* MemberController = NewObject<USFPartyMemberWidgetController>(this, PartyMemberWidgetControllerClass );
    
	// 원격 PlayerState의 데이터로 파라미터 구성
	const FWidgetControllerParams Params
	(
		nullptr, // PC 사용 x
		PlayerState, // 원격 PS
		PlayerState->GetAbilitySystemComponent(), // 원격 ASC
		const_cast<USFPrimarySet_Hero*>(PlayerState->GetPrimarySet()), // 원격 AttributeSet
		const_cast<USFCombatSet_Hero*>(PlayerState->GetCombatSet()), // 원격 AttributeSet
		GameState
	);
    
	MemberController->SetWidgetControllerParams(Params);
	MemberController->BindCallbacksToDependencies();

	PartyMemberControllerMap.Add(PlayerState, MemberController);

	PlayerState->OnPlayerInfoChanged.AddDynamic(this, &USFPartyWidgetController::HandlePlayerInfoChangedForReorder);
	
	OnPartyMemberAdded.Broadcast(MemberController);
	OnPartyOrderChanged.Broadcast();

	// 위젯에 위젯 컨트롤러 설정(WBP_PartyInfo에서 호출함)
	// 생성된 파티 멤버 컨트롤러가 초기값을 브로드캐스트하도록 호출(BP에서 호출함)
}

void USFPartyWidgetController::RemoveAndBroadcastMember(ASFPlayerState* PlayerState)
{
	USFPartyMemberWidgetController* MemberController = PartyMemberControllerMap.FindRef(PlayerState);
	if (MemberController)
	{
		PlayerState->OnPlayerInfoChanged.RemoveDynamic(this, &USFPartyWidgetController::HandlePlayerInfoChangedForReorder);
		PartyMemberControllerMap.Remove(PlayerState);
		OnPartyMemberRemoved.Broadcast(MemberController);

		// MemberController는 TMap에서 제거되었으므로 GC됨
	}
}

USFPartyMemberWidgetController* USFPartyWidgetController::GetMemberController(APlayerState* PlayerState) const
{
	if (const TObjectPtr<USFPartyMemberWidgetController>* Controller = PartyMemberControllerMap.Find(PlayerState))
	{
		return *Controller;
	}
	return nullptr;
}