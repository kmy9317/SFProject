#include "SFPlayerState.h"

#include "SFLogChannels.h"
#include "SFPlayerController.h"
#include "SFPlayerInfoTypes.h"
#include "AbilitySystem/SFAbilitySet.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Hero/SFCombatSet_Hero.h"
#include "AbilitySystem/Attributes/Hero/SFPrimarySet_Hero.h"
#include "Character/SFPawnData.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Character/Hero/SFHeroDefinition.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Messages/SFMessageGameplayTags.h"
#include "Messages/SFPortalInfoMessages.h"
#include "Net/UnrealNetwork.h"
#include "System/SFAssetManager.h"

ASFPlayerState::ASFPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<USFAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	PrimarySet = CreateDefaultSubobject<USFPrimarySet_Hero>(TEXT("PrimarySet"));
	CombatSet = CreateDefaultSubobject<USFCombatSet_Hero>(TEXT("CombatSet"));
	
	SetNetUpdateFrequency(100.f);
}

void ASFPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, PlayerSelection);
	DOREPLIFETIME(ThisClass, bIsReadyForTravel);
	
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyPlayerConnectionType, SharedParams)
}

void ASFPlayerState::Reset()
{
	Super::Reset();
}

void ASFPlayerState::ClientInitialize(AController* C)
{
	// PlayerController에 PlayerState 설정 시점에 호출되는 함수(OnRep_PlayerState)
	Super::ClientInitialize(C);
	if (USFPawnExtensionComponent* PawnExtensionComponent = USFPawnExtensionComponent::FindPawnExtensionComponent(GetPawn()))
	{
		UE_LOG(LogSF, Warning, TEXT("client init"));
		PawnExtensionComponent->CheckDefaultInitialization();
	}
}

void ASFPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	ASFPlayerState* NewPlayerState = Cast<ASFPlayerState>(PlayerState);
	if (NewPlayerState)
	{
		NewPlayerState->SetPlayerSelection(PlayerSelection);
	}
}

void ASFPlayerState::OnDeactivated()
{
	bool bDestroyDeactivatedPlayerState = false;

	switch (GetPlayerConnectionType())
	{
	case ESFPlayerConnectionType::Player:
	case ESFPlayerConnectionType::InactivePlayer:
		//@TODO: Ask the experience if we should destroy disconnecting players immediately or leave them around
		// (e.g., for long running servers where they might build up if lots of players cycle through)
		bDestroyDeactivatedPlayerState = true;
		break;
	default:
		bDestroyDeactivatedPlayerState = true;
		break;
	}
	
	SetPlayerConnectionType(ESFPlayerConnectionType::InactivePlayer);

	if (bDestroyDeactivatedPlayerState)
	{
		Destroy();
	}
}

void ASFPlayerState::OnReactivated()
{
	if (GetPlayerConnectionType() == ESFPlayerConnectionType::InactivePlayer)
	{
		SetPlayerConnectionType(ESFPlayerConnectionType::Player);
	}
}

ASFPlayerController* ASFPlayerState::GetSFPlayerController() const
{
	return Cast<ASFPlayerController>(GetOwner());
}

USFAbilitySystemComponent* ASFPlayerState::GetSFAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAbilitySystemComponent* ASFPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASFPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	//check(AbilitySystemComponent); // ASC가 연결되기 전이므로 필요없음
	//AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn()); // Pawn이 null일 가능성 있어 주석 처리 (HeroComponent에서 진행)

	// TODO : PawnData등의 로드 완료 Delegate 바인딩
	// 서버에서만 PawnData 로드 관리
	UWorld* World = GetWorld();
	if (World && World->IsGameWorld() && World->GetNetMode() != NM_Client)
	{
		// GameMode가 HandleSeamlessTravelPlayer에서 호출할 예정
		// 여기서는 델리게이트만 준비
	}
}

void ASFPlayerState::StartLoadingPawnData()
{
	if (!HasAuthority() || bPawnDataLoaded)
	{
		return;
	}
    
	// CharacterDefinition에서 PawnData 가져오기
	if (PlayerSelection.GetHeroDefinition())
	{
		TSoftObjectPtr<USFPawnData> PawnDataPath = PlayerSelection.GetHeroDefinition()->GetPawnDataPath();
        
		if (!PawnDataPath.IsNull())
		{
			UE_LOG(LogSF, Log, TEXT("Starting async load of PawnData for player %s"), *GetPlayerName());
            
			// 비동기 로드 시작
			FStreamableDelegate OnLoaded = FStreamableDelegate::CreateLambda([this, PawnDataPath]()
			{
				if (USFPawnData* LoadedPawnData = PawnDataPath.Get())
				{
					OnPawnDataLoadComplete(LoadedPawnData);
				}
			});
			PawnDataHandle = USFAssetManager::Get().LoadPawnDataAsync(PawnDataPath, OnLoaded);
		}
		else
		{
			// PawnData가 없으면 기본값으로
			OnPawnDataLoadComplete(GetDefault<USFPawnData>());
		}
	}
}

void ASFPlayerState::OnPawnDataLoadComplete(const USFPawnData* LoadedPawnData)
{
	UE_LOG(LogSF, Log, TEXT("PawnData load complete for player %s"), *GetPlayerName());
    
	// PawnData 설정
	if (LoadedPawnData)
	{
		SetPawnData(LoadedPawnData);
	}
    
	bPawnDataLoaded = true;
	PawnDataHandle.Reset();
    
	// 델리게이트 브로드캐스트 - GameMode가 처리
	OnPawnDataLoaded.Broadcast(LoadedPawnData);
}

void ASFPlayerState::SetPawnData(const USFPawnData* InPawnData)
{
	check(InPawnData);

	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (PawnData)
	{
		UE_LOG(LogSF, Error, TEXT("Trying to set PawnData [%s] on player state [%s] that already has valid PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(this), *GetNameSafe(PawnData));
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PawnData, this);
	PawnData = InPawnData;

	// PawnData의 AbilitySet을 순회하며, ASC에 Ability를 할당
	// - 이 과정에서 ASC의 ActivatableAbilities에 추가됨
	for (const USFAbilitySet* AbilitySet : PawnData->AbilitySets)
	{
		if (AbilitySet)
		{
			AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
		}
	}

	ForceNetUpdate();
}

void ASFPlayerState::SetPlayerSelection(const FSFPlayerSelectionInfo& NewPlayerSelection)
{
	PlayerSelection = NewPlayerSelection;
	
	if (HasAuthority())
	{
		OnRep_PlayerSelection();
	}
}

void ASFPlayerState::SetIsReadyForTravel(bool bInIsReadyForTravel)
{
	bIsReadyForTravel = bInIsReadyForTravel;

	if (HasAuthority())
	{
		OnRep_IsReadyForTravel();
	}
}

void ASFPlayerState::SetPlayerConnectionType(ESFPlayerConnectionType NewType)
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MyPlayerConnectionType, this);
	MyPlayerConnectionType = NewType;
}

void ASFPlayerState::OnRep_PlayerSelection()
{
	OnPlayerInfoChanged.Broadcast(PlayerSelection);
}

void ASFPlayerState::OnRep_IsReadyForTravel()
{
	// 클라이언트의 로컬 GMS에 개별 플레이어별 준비 상태 변경 메시지를 브로드캐스트
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	FSFPlayerTravelReadyMessage Message;
	Message.PlayerState = this; 
	Message.bIsReadyToTravel = bIsReadyForTravel;
	MessageSubsystem.BroadcastMessage(SFGameplayTags::Message_Player_TravelReadyChanged, Message);
}
