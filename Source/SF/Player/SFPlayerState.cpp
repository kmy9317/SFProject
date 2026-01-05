#include "SFPlayerState.h"

#include "GenericTeamAgentInterface.h"
#include "SFLogChannels.h"
#include "SFPlayerController.h"
#include "SFPlayerInfoTypes.h"
#include "AbilitySystem/SFAbilitySet.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "AbilitySystem/Attributes/Hero/SFCombatSet_Hero.h"
#include "AbilitySystem/Attributes/Hero/SFPrimarySet_Hero.h"
#include "Character/SFPawnData.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Character/Hero/SFHeroDefinition.h"
#include "Components/SFCommonUpgradeComponent.h"
#include "Components/SFPlayerCombatStateComponent.h"
#include "Components/SFPlayerStatsComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Messages/SFMessageGameplayTags.h"
#include "Messages/SFPortalInfoMessages.h"
#include "Net/UnrealNetwork.h"
#include "System/SFAssetManager.h"

ASFPlayerState::ASFPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Ability
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<USFAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// Hero Attribute
	PrimarySet = CreateDefaultSubobject<USFPrimarySet_Hero>(TEXT("PrimarySet"));
	CombatSet = CreateDefaultSubobject<USFCombatSet_Hero>(TEXT("CombatSet"));

	// Upgrade
	PermanentUpgradeComponent = CreateDefaultSubobject<USFPermanentUpgradeComponent>(TEXT("PermanentUpgradeComponent"));

	// Common Upgrade
	CommonUpgradeComponent = CreateDefaultSubobject<USFCommonUpgradeComponent>(TEXT("CommonUpgradeComponent"));

	// CombatState
	CombatStateComponent = CreateDefaultSubobject<USFPlayerCombatStateComponent>(TEXT("CombatStateComponent"));

	// player Stats
	StatsComponent = CreateDefaultSubobject<USFPlayerStatsComponent>(TEXT("StatsComponent"));
	
	SetNetUpdateFrequency(100.f);

	// TODO : 테스트용 삭제 예정
	Gold = 500;
}

void ASFPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PawnDataHandle.IsValid())
	{
		PawnDataHandle->CancelHandle();
		PawnDataHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void ASFPlayerState::PostNetInit()
{
	Super::PostNetInit();

	if (CombatStateComponent)
	{
		CombatStateComponent->MarkInitialDataReceived();
	}
}

void ASFPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, PlayerSelection);
	DOREPLIFETIME(ThisClass, PawnData);
	DOREPLIFETIME(ThisClass, Gold);
	DOREPLIFETIME(ThisClass, bIsReadyForTravel);
	DOREPLIFETIME(ThisClass, PermanentUpgradeData);
	
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyPlayerConnectionType, SharedParams)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyTeamID, SharedParams);

	SharedParams.Condition = ELifetimeCondition::COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ReplicatedViewRotation, SharedParams);
}

void ASFPlayerState::Reset()
{
	Super::Reset();
}

void ASFPlayerState::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MyTeamID, this);
		MyTeamID = NewTeamID;
	}
}

FGenericTeamId ASFPlayerState::GetGenericTeamId() const
{
	return MyTeamID;
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
	if (!NewPlayerState)
	{
		return;
	}

	NewPlayerState->SetPlayerSelection(PlayerSelection);
	NewPlayerState->SetGenericTeamId(MyTeamID);

	// Permanent Upgrade 데이터도 SeamlessTravel/InactivePlayer에 이어받도록 복사
	NewPlayerState->bPermanentUpgradeAppliedThisGame = bPermanentUpgradeAppliedThisGame;
	NewPlayerState->PermanentUpgradeData = PermanentUpgradeData;
	NewPlayerState->bPermanentUpgradeDataReceived = bPermanentUpgradeDataReceived;
	
	if (SavedASCData.IsValid())
	{
		NewPlayerState->SavedASCData = SavedASCData;
	}

	if (CombatStateComponent && NewPlayerState->CombatStateComponent)
	{
		NewPlayerState->CombatStateComponent->RestoreCombatStateFromTravel(CombatStateComponent->GetCombatInfo());
	}

	if (StatsComponent && NewPlayerState->StatsComponent)
	{
		NewPlayerState->StatsComponent->CopyStatsFrom(StatsComponent);
	}
	
	// TODO : 테스트용 삭제 예정
	NewPlayerState->Gold = Gold;

	// TODO: Lobby로 진입시에 넘겨줄 데이터 지정
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

bool ASFPlayerState::IsDead() const
{
	if (CombatStateComponent)
	{
		return CombatStateComponent->IsDead();
	}
	return false;
}

void ASFPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());

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

			TWeakObjectPtr<ASFPlayerState> WeakThis(this);
            
			// 비동기 로드 시작
			FStreamableDelegate OnLoaded = FStreamableDelegate::CreateLambda([WeakThis, PawnDataPath]()
			{
				if (ASFPlayerState* StrongThis = WeakThis.Get())
				{
					if (USFPawnData* LoadedPawnData = PawnDataPath.Get())
					{
						StrongThis->OnPawnDataLoadComplete(LoadedPawnData);
					}
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
	
	TryApplyPermanentUpgrade();
	
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

	PawnData = InPawnData;

	const bool bHasSavedAbilities = HasSavedAbilitySystemData();
	
	if (bHasSavedAbilities)
	{
		RestorePersistedAbilityData();
	}
	else
	{
		// PawnData의 AbilitySet을 순회하며, ASC에 Ability를 할당
		// - 이 과정에서 ASC의 ActivatableAbilities에 추가됨
		for (const USFAbilitySet* AbilitySet : PawnData->AbilitySets)
		{
			if (AbilitySet)
			{
				AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
			}
		}

		// 강화는 "서버가 PlayFab 데이터를 수신한 이후"에만 적용되어야 함
		//TryApplyPermanentUpgrade();
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

FRotator ASFPlayerState::GetReplicatedViewRotation() const
{
	return ReplicatedViewRotation;
}

void ASFPlayerState::SetReplicatedViewRotation(const FRotator& NewRotation)
{
	if (NewRotation != ReplicatedViewRotation)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ReplicatedViewRotation, this);
		ReplicatedViewRotation = NewRotation;
	}
}

void ASFPlayerState::SavePersistedData()
{
	if (AbilitySystemComponent)
	{
		SavedASCData.Reset();
		AbilitySystemComponent->SaveAttributesToData(SavedASCData);
		AbilitySystemComponent->SaveAbilitiesToData(SavedASCData);
		AbilitySystemComponent->SaveGameplayEffectsToData(SavedASCData);
	}
}

void ASFPlayerState::RestorePersistedAbilityData()
{
	if (!SavedASCData.IsValid())
	{
		UE_LOG(LogSF, Log, TEXT("RestorePersistedAbilityData: No saved data for %s"), *GetPlayerName());
		return;
	}

	if (!AbilitySystemComponent)
	{
		return;
	}

	// ASC 데이터 복원
	AbilitySystemComponent->RestoreAttributesFromData(SavedASCData);
	AbilitySystemComponent->RestoreAbilitiesFromData(SavedASCData);
	AbilitySystemComponent->RestoreGameplayEffectsFromData(SavedASCData);

	// 클라이언트에 강제 동기화
	AbilitySystemComponent->ForceReplication();

	// 버퍼 비우기
	SavedASCData.Reset();
}

void ASFPlayerState::Server_RequestSkillUpgrade_Implementation(TSubclassOf<USFGameplayAbility> NewAbilityClass, FGameplayTag InputTag)
{
	if (!NewAbilityClass || !InputTag.IsValid())
	{
		UE_LOG(LogSF, Warning, TEXT("Server_RequestSkillUpgrade: Invalid parameters"));
		return;
	}

	// 유효성 검증: PawnData의 UpgradeOptions에 해당 어빌리티가 있는지 확인
	if (PawnData)
	{
		TArray<TSubclassOf<USFGameplayAbility>> ValidOptions = PawnData->GetUpgradeOptionsForSlot(InputTag);
		if (!ValidOptions.Contains(NewAbilityClass))
		{
			return;
		}
	}

	ApplySkillUpgrade(NewAbilityClass, InputTag);
}

void ASFPlayerState::ApplySkillUpgrade(TSubclassOf<USFGameplayAbility> NewAbilityClass, FGameplayTag InputTag)
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (!AbilitySystemComponent)
	{
		return;
	}

	// 기존 어빌리티에서 레벨 가져오고 제거
	int32 InheritedLevel = 1;
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			InheritedLevel = Spec.Level;
			AbilitySystemComponent->ClearAbility(Spec.Handle);
			UE_LOG(LogSF, Log, TEXT("Removed old ability, inherited level: %d"), InheritedLevel);
			break;
		}
	}

	// 새 어빌리티 부여 (레벨 계승)
	USFGameplayAbility* AbilityCDO = NewAbilityClass->GetDefaultObject<USFGameplayAbility>();
	FGameplayAbilitySpec NewSpec(AbilityCDO, InheritedLevel);
	NewSpec.GetDynamicSpecSourceTags().AddTag(InputTag);
    
	AbilitySystemComponent->GiveAbility(NewSpec);
}

void ASFPlayerState::OnRep_PawnData()
{
	if (PawnData)
	{
		bPawnDataLoaded = true;

		OnPawnDataLoaded.Broadcast(PawnData);
	}
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


void ASFPlayerState::SetPermanentUpgradeData(const FSFPermanentUpgradeData& InData)
{
	PermanentUpgradeData = InData;

	UE_LOG(LogTemp, Warning, TEXT("[PermanentUpgrade] SetPermanentUpgradeData CALLED | Wrath=%d Pride=%d Lust=%d Sloth=%d Greed=%d"),
		InData.Wrath, InData.Pride, InData.Lust, InData.Sloth, InData.Greed);

	if (!HasAuthority())
	{
		return;
	}

	// 값이 0이어도 "데이터 수신 완료"로 취급해야 함
	bPermanentUpgradeDataReceived = true;

	TryApplyPermanentUpgrade();
}
void ASFPlayerState::Server_SubmitPermanentUpgradeData_Implementation(const FSFPermanentUpgradeData& InData)
{
	// Owning Client → Server
	SetPermanentUpgradeData(InData);
}

bool ASFPlayerState::ArePermanentUpgradeDataEqual(const FSFPermanentUpgradeData& A, const FSFPermanentUpgradeData& B)
{
	return A.Wrath == B.Wrath
		&& A.Pride == B.Pride
		&& A.Lust == B.Lust
		&& A.Sloth == B.Sloth
		&& A.Greed == B.Greed;
}

void ASFPlayerState::TryApplyPermanentUpgrade()
{
	UE_LOG(LogTemp, Warning, TEXT("[PermanentUpgrade] TryApply ENTER | this=%p PlayerId=%d Name=%s AppliedThisGame=%d"),
	this, GetPlayerId(), *GetPlayerName(), bPermanentUpgradeAppliedThisGame ? 1 : 0);

	//이미 이번 게임에서 적용됐다면 무조건 스킵
	if (bPermanentUpgradeAppliedThisGame)
	{
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[PermanentUpgrade] TryApply | Auth=%d Received=%d ASC=%s Avatar=%s Comp=%s"),
		HasAuthority() ? 1 : 0,
		bPermanentUpgradeDataReceived ? 1 : 0,
		AbilitySystemComponent ? TEXT("Y") : TEXT("N"),
		(AbilitySystemComponent && AbilitySystemComponent->GetAvatarActor()) ? TEXT("Y") : TEXT("N"),
		PermanentUpgradeComponent ? TEXT("Y") : TEXT("N")
	);
	
	if (!HasAuthority())
	{
		return;
	}

	// ★ 핵심: 서버가 "데이터를 받았다"는 사실이 먼저여야 함
	if (!bPermanentUpgradeDataReceived)
	{
		return;
	}

	if (!AbilitySystemComponent || !PermanentUpgradeComponent)
	{
		return;
	}

	if (!AbilitySystemComponent->GetAvatarActor())
	{
		SchedulePermanentUpgradeRetry();
		return;
	}
	
	if (bHasLastAppliedPermanentUpgradeData && ArePermanentUpgradeDataEqual(LastAppliedPermanentUpgradeData, PermanentUpgradeData))
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[PermanentUpgrade] TryApply | Wrath=%d Pride=%d Lust=%d Sloth=%d Greed=%d"),
		PermanentUpgradeData.Wrath,
		PermanentUpgradeData.Pride,
		PermanentUpgradeData.Lust,
		PermanentUpgradeData.Sloth,
		PermanentUpgradeData.Greed);

	PermanentUpgradeComponent->ApplyUpgradeBonuses(AbilitySystemComponent, PermanentUpgradeData);

	bHasLastAppliedPermanentUpgradeData = true;
	LastAppliedPermanentUpgradeData = PermanentUpgradeData;

	GetWorld()->GetTimerManager().ClearTimer(PermanentUpgradeRetryTimer);
	bPermanentUpgradeAppliedThisGame = true;
}

void ASFPlayerState::SchedulePermanentUpgradeRetry()
{
	UE_LOG(LogTemp, Error, TEXT("[PermanentUpgrade] ScheduleRetry"));
	
	if (!GetWorld())
	{
		return;
	}

	// 이미 예약돼 있으면 중복 예약 금지
	if (GetWorld()->GetTimerManager().IsTimerActive(PermanentUpgradeRetryTimer))
	{
		return;
	}

	// 0.2초 후 재시도 (필요시 0.1~0.5 사이로 조절)
	GetWorld()->GetTimerManager().SetTimer(
		PermanentUpgradeRetryTimer,
		this,
		&ASFPlayerState::TryApplyPermanentUpgrade,
		0.2f,
		false
	);
}