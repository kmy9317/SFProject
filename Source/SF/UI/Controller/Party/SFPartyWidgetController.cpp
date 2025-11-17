// Fill out your copyright notice in the Description page of Project Settings.


#include "SFPartyWidgetController.h"

#include "SFLogChannels.h"
#include "SFPartyMemberWidgetController.h"
#include "GameModes/SFGameState.h"
#include "Player/SFPlayerState.h"
#include "UI/SFUserWidget.h"

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
	// 현재 GameState의 PlayerArray를 순회하며 초기 목록 브로드캐스트
	for (APlayerState* PS : SFGameState->PlayerArray)
	{
		HandlePlayerAdded(PS);
	}
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

void USFPartyWidgetController::AddAndBroadcastMember(ASFPlayerState* PlayerState)
{
	// TMap에 이미 존재하는지 확인 
	if (PartyMemberControllers.Contains(PlayerState))
	{
		UE_LOG(LogSF, Warning, TEXT("USFPartyWidgetController: PlayerState is already in PartyMemberControllers."));
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

	// TMap에 저장
	PartyMemberControllers.Add(PlayerState, MemberController);
	
	// 위젯에 위젯 컨트롤러 설정
	
	// 위젯 컨트롤러가 초기값을 브로드캐스트하도록 호출
	
	OnPartyMemberAdded.Broadcast(MemberController);
}

void USFPartyWidgetController::RemoveAndBroadcastMember(ASFPlayerState* PlayerState)
{
	USFPartyMemberWidgetController* MemberController = PartyMemberControllers.FindRef(PlayerState);
	if (MemberController)
	{
		PartyMemberControllers.Remove(PlayerState);
		OnPartyMemberRemoved.Broadcast(MemberController);

		// MemberController는 TMap에서 제거되었으므로 GC됨
	}
}

USFPartyMemberWidgetController* USFPartyWidgetController::GetMemberController(APlayerState* PlayerState) const
{
	if (const TObjectPtr<USFPartyMemberWidgetController>* Controller = PartyMemberControllers.Find(PlayerState))
	{
		return *Controller;
	}
	return nullptr;
}