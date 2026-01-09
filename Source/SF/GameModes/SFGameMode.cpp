#include "SFGameMode.h"

#include "EngineUtils.h"
#include "SFEnemyManagerComponent.h"
#include "SFGameOverManagerComponent.h"
#include "SFGameState.h"
#include "SFLogChannels.h"
#include "SFPortalManagerComponent.h"
#include "SFStageManagerComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "System/SFPlayFabSubsystem.h"
#include "Player/SFPlayerInfoTypes.h"
#include "Player/SFPlayerState.h"
#include "Character/Hero/SFHeroDefinition.h"
#include "Character/SFPawnData.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Engine/PlayerStartPIE.h"
#include "Player/SFPlayerController.h"
#include "System/SFGameInstance.h"

ASFGameMode::ASFGameMode()
{
	bUseSeamlessTravel = true;
}

void ASFGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	
	AssignedPlayerStarts.Empty();
}

void ASFGameMode::InitGameState()
{
	Super::InitGameState();
	// TODO : SFGameState 캐싱

	// EnemyManager 델리게이트 바인딩
	if (ASFGameState* SFGameState = GetGameState<ASFGameState>())
	{
		if (USFEnemyManagerComponent* EnemyManager = SFGameState->GetEnemyManager())
		{
			EnemyManager->OnAllEnemiesDefeated.AddDynamic(this, &ThisClass::OnAllEnemiesDefeated);
		}

		if (USFGameOverManagerComponent* GameOverManager = SFGameState->GetGameOverManager())
		{
			GameOverManager->OnGameOver.AddDynamic(this, &ThisClass::OnGameOver);
		}
	}
}

void ASFGameMode::StartPlay()
{
	Super::StartPlay();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{
			if (ASFGameState* SFGameState = GetGameState<ASFGameState>())
			{
				if (USFEnemyManagerComponent* EnemyManager = SFGameState->GetEnemyManager())
				{
					EnemyManager->NotifyAllEnemiesSpawned();
				}
			}
		});
	}

	if (!bPermanentUpgradeFlowStarted) // ← 네 프로젝트의 1-1 판별 로직
	{
		UE_LOG(LogTemp, Warning, TEXT("[PermanentUpgrade] Game START stage"));

		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (ASFPlayerController* PC = Cast<ASFPlayerController>(It->Get()))
			{
				PC->Client_BeginPermanentUpgradeFlow();
			}
		}
	}
}

void ASFGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// PIE 테스트 모드 체크
	if (bUsePIETestMode && GetWorld()->IsPlayInEditor())
	{
		if (PIETestHeroDefinition)  // TObjectPtr 직접 체크
		{
			ASFPlayerState* SFPS = NewPlayer->GetPlayerState<ASFPlayerState>();
			if (SFPS)
			{
				FSFPlayerSelectionInfo TestSelection(0, SFPS);
				TestSelection.SetHeroDefinition(PIETestHeroDefinition);
				SFPS->SetPlayerSelection(TestSelection);
                
				UE_LOG(LogSF, Log, TEXT("PIE Test Mode: Applied [%s]"), 
					*PIETestHeroDefinition->GetHeroDisplayName());
			}
		}
	}

	if (ASFPlayerState* PS = NewPlayer->GetPlayerState<ASFPlayerState>())
	{
		// 기본 팀 설정
		PS->SetGenericTeamId(FGenericTeamId(SFTeamID::Player));

		// PermanentUpgradeData는 클라이언트에서 PlayFab 로드 완료 후 Server RPC로 전송됨
		// (서버에서 GameInstance Subsystem을 읽어 적용하면 플레이어별 값이 구분되지 않음)
	}
	
	// 공통 함수 호출 (Seamless Travel과 동일한 로직)
	SetupPlayerPawnDataLoading(NewPlayer);
}

void ASFGameMode::HandleSeamlessTravelPlayer(AController*& Controller)
{
	Super::HandleSeamlessTravelPlayer(Controller);

	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC)
	{
		return;
	}

	SetupPlayerPawnDataLoading(PC);
}

void ASFGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// PawnData 로드될 때까지 대기
	if (ASFPlayerState* SFPS = NewPlayer->GetPlayerState<ASFPlayerState>())
	{
		if (SFPS->IsPawnDataLoaded())
		{
			Super::HandleStartingNewPlayer_Implementation(NewPlayer);
		}
		// 로드 중이면 OnPlayerPawnDataLoaded에서 처리
	}

	//서버만
	if (!HasAuthority())
	{
		return;
	}

	// 클라이언트 여부에 따라 분기 처리
	if (NewPlayer->IsLocalController())
	{
		// 1. 호스트(Listen Server) 또는 Standalone인 경우
		// 기존 로직 유지: 로컬 Subsystem을 직접 호출
		if (UGameInstance* GI = GetGameInstance())
		{
			if (USFPlayFabSubsystem* PF = GI->GetSubsystem<USFPlayFabSubsystem>())
			{
				PF->TryStartPermanentUpgradeForThisGame();
			}
		}
	}
	else
	{
		// 2. 접속한 클라이언트(Remote)인 경우
		// 클라이언트에게 "데이터 전송을 시작하라"는 RPC를 보내야 함
		if (ASFPlayerController* SFPC = Cast<ASFPlayerController>(NewPlayer))
		{
			// SFPlayerController에 이 함수가 있다고 가정 (StartPlay에서 호출하던 함수)
			SFPC->Client_BeginPermanentUpgradeFlow();
			UE_LOG(LogSF, Log, TEXT("[GameMode] Triggered Client_BeginPermanentUpgradeFlow for %s"), *NewPlayer->GetName());
		}
	}
}

UClass* ASFGameMode::GetDefaultPawnClassForController_Implementation(AController* Controller)
{
	if (const USFPawnData* PawnData = GetPawnDataForController(Controller))
	{
		if (PawnData->PawnClass)
		{
			return PawnData->PawnClass;
		}
	}

	return nullptr;
}

APawn* ASFGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;
	SpawnInfo.bDeferConstruction = true;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    
	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo))
		{
			// PawnExtensionComponent에 PawnData 설정
			if (USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(SpawnedPawn))
			{
				if (const USFPawnData* PawnData = GetPawnDataForController(NewPlayer))
				{
					PawnExtComp->SetPawnData(PawnData);
				}
				else
				{
					UE_LOG(LogSF, Error, TEXT("Game mode was unable to set PawnData on the spawned pawn [%s]."), 
						*GetNameSafe(SpawnedPawn));
				}
			}
            
			SpawnedPawn->FinishSpawning(SpawnTransform);
            
			UE_LOG(LogSF, Log, TEXT("Successfully spawned pawn for player %s"), 
				*GetNameSafe(NewPlayer));
            
			return SpawnedPawn;
		}
	}
    
	return nullptr;
}

bool ASFGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	// PawnData 로드 체크
	if (ASFPlayerState* SFPS = Player->GetPlayerState<ASFPlayerState>())
	{
		if (!SFPS->IsPawnDataLoaded())
		{
			UE_LOG(LogSF, Verbose, TEXT("PlayerCanRestart: false - PawnData not loaded"));
			return false;
		}

		if (SFPS->IsDead())
		{
			return false;
		}
	}
	
	return Super::PlayerCanRestart_Implementation(Player);
}

AActor* ASFGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	UWorld* World = GetWorld();
	UClass* PawnClass = GetDefaultPawnClassForController(Player);
	APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;

	TArray<APlayerStart*> AvailableStarts;
	
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		APlayerStart* PlayerStart = *It;

		// 에디터에서 Play From Here로 테스트 용이하게 함
		if (PlayerStart->IsA<APlayerStartPIE>())
		{
			return PlayerStart;
		}

		if (AssignedPlayerStarts.Contains(PlayerStart))
		{
			continue;
		}

		FVector ActorLocation = PlayerStart->GetActorLocation();
		const FRotator ActorRotation = PlayerStart->GetActorRotation();
        
		if (!World->EncroachingBlockingGeometry(PawnToFit, ActorLocation, ActorRotation))
		{
			AvailableStarts.Add(PlayerStart);
		}
	}
	
	// 태그 기준 정렬
	AvailableStarts.Sort([](const APlayerStart& A, const APlayerStart& B)
	{
		int32 IndexA = FCString::Atoi(*A.PlayerStartTag.ToString());
		int32 IndexB = FCString::Atoi(*B.PlayerStartTag.ToString());
		return IndexA < IndexB;
	});

	APlayerStart* ChosenStart = AvailableStarts.Num() > 0 ? AvailableStarts[0] : nullptr;
    
	if (AvailableStarts.Num() > 0)
	{
		ChosenStart = AvailableStarts[0];
		AssignedPlayerStarts.Add(ChosenStart);
	}
	else if (AssignedPlayerStarts.Num() > 0)
	{
		// 부족하면 첫 번째 사용했던 것 재사용
		ChosenStart = AssignedPlayerStarts[0];
	}

	return ChosenStart;
}

const USFPawnData* ASFGameMode::GetPawnDataForController(const AController* InController) const
{
	if (InController)
	{
		if (const ASFPlayerState* SFPS = InController->GetPlayerState<ASFPlayerState>())
		{
			if (!SFPS->IsPawnDataLoaded())
			{
				return nullptr;
			}
			// PlayerState의 PawnData 반환
			if (const USFPawnData* PawnData = SFPS->GetPawnData<USFPawnData>())
			{
				return PawnData;
			}
            
			// 만약 로드가 안됐으면 동기 로딩 해보기
			const FSFPlayerSelectionInfo& Selection = SFPS->GetPlayerSelection();
			if (const USFHeroDefinition* HeroDef = Selection.GetHeroDefinition())
			{
				return HeroDef->GetPawnData();
			}
		}
	}
    
	return nullptr;
}


void ASFGameMode::SetupPlayerPawnDataLoading(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}
    
	ASFPlayerState* SFPS = PC->GetPlayerState<ASFPlayerState>();
	if (!SFPS)
	{
		return;
	}
    
	UE_LOG(LogSF, Log, TEXT("Setting up PawnData loading for %s"), *SFPS->GetPlayerName());
    
	// 로드 완료 콜백 등록
	if (!SFPS->IsPawnDataLoaded())
	{
		PendingPlayers.Add(PC);
        
		TWeakObjectPtr<APlayerController> WeakPC = PC;
		TWeakObjectPtr<ASFGameMode> WeakThis = this;
        
		SFPS->OnPawnDataLoaded.AddLambda([WeakThis, WeakPC](const USFPawnData* PawnData)
		{
			if (ASFGameMode* GameMode = WeakThis.Get())
			{
				if (APlayerController* ValidPC = WeakPC.Get())
				{
					GameMode->OnPlayerPawnDataLoaded(ValidPC, PawnData);
				}
			}
		});
        
		// 비동기 로드 시작
		SFPS->StartLoadingPawnData();
	}
	else
	{
		// 이미 로드된 경우 바로 스폰
		if (!PC->GetPawn() && PlayerCanRestart(PC))
		{
			RestartPlayer(PC);
		}
	}
}

void ASFGameMode::OnPlayerPawnDataLoaded(APlayerController* PC, const USFPawnData* PawnData)
{
	if (!PC || !IsValid(PC))
	{
		return;
	}
    
	UE_LOG(LogSF, Log, TEXT("PawnData loaded for player, attempting restart"));
    
	PendingPlayers.Remove(PC);
    
	// 로드 완료된 플레이어 개별 스폰
	if (!PC->GetPawn() && PlayerCanRestart(PC))
	{
		RestartPlayer(PC);
	}
}

void ASFGameMode::ActivatePortal()
{
	if (ASFGameState* SFGameState = GetGameState<ASFGameState>())
	{
		if (USFPortalManagerComponent* PortalManager = SFGameState->GetPortalManager())
		{
			PortalManager->ActivatePortal();
		}
	}
}

void ASFGameMode::NotifyStageClear()
{
	if (ASFGameState* SFGameState = GetGameState<ASFGameState>())
	{
		if (USFStageManagerComponent* StageManager = SFGameState->GetStageManager())
		{
			StageManager->NotifyStageClear();
		}
	}
}

void ASFGameMode::RequestTravelToNextStage(TSoftObjectPtr<UWorld> NextStageLevel)
{
	if (NextStageLevel.IsNull())
	{
		UE_LOG(LogSF, Error, TEXT("[GameMode] NextStageLevel is not set!"));
		return;
	}

	// 트래블 시작 전 모든 플레이어의 ASC 데이터를 백업
	if (GameState)
	{
		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (ASFPlayerState* SFPS = Cast<ASFPlayerState>(PS))
			{
				SFPS->SavePersistedData();
			}
		}
	}

	bPermanentUpgradeFlowStarted = true;
	
	UE_LOG(LogSF, Log, TEXT("[GameMode] Traveling to next stage: %s"), *NextStageLevel.ToString());

	if (USFGameInstance* SFGameInstance = Cast<USFGameInstance>(GetWorld()->GetGameInstance()))
	{
		SFGameInstance->LoadLevelAndListen(NextStageLevel);
	}
}

void ASFGameMode::RequestTravelToLobby(TSoftObjectPtr<UWorld> LobbyLevel)
{
	if (LobbyLevel.IsNull())
	{
		UE_LOG(LogSF, Error, TEXT("[GameMode] LobbyLevel is not set!"));
		return;
	}

	// TODO : 로비 이동전 필요한 플레이어별 획득한 데이터 처리?

	UE_LOG(LogSF, Log, TEXT("[GameMode] Traveling to lobby: %s"), *LobbyLevel.ToString());

	if (USFGameInstance* SFGameInstance = Cast<USFGameInstance>(GetWorld()->GetGameInstance()))
	{
		SFGameInstance->LoadLevelAndListen(LobbyLevel);
	}
}

void ASFGameMode::OnAllEnemiesDefeated()
{
	UE_LOG(LogSF, Warning, TEXT("[GameMode] Stage cleared! Activating portal..."));

	if (IsBossStage())
	{
		ReviveAllIncapacitatedPlayers();
	}

	// 최종 스테이지 체크
	if (IsFinalStage())
	{
		if (ASFGameState* SFGS = GetGameState<ASFGameState>())
		{
			if (USFGameOverManagerComponent* GameOverManager = SFGS->GetGameOverManager())
			{
				GameOverManager->TriggerGameClear();
			}
		}
		return;
	}

	NotifyStageClear();
	ActivatePortal();
}

void ASFGameMode::ReviveAllIncapacitatedPlayers()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (ASFPlayerState* SFPS = PC->GetPlayerState<ASFPlayerState>())
			{
				USFPlayerCombatStateComponent* CombatComp = SFPS->FindComponentByClass<USFPlayerCombatStateComponent>();
				if (!CombatComp)
				{
					continue;
				}

				if (CombatComp->IsDead())
				{
					ReviveDeadPlayer(SFPS);
				}
				else if (CombatComp->IsDowned())
				{
					ReviveDownedPlayer(SFPS);
				}
			}
		}
	}
}

void ASFGameMode::ReviveDeadPlayer(ASFPlayerState* PlayerState)
{
    if (!PlayerState || !HasAuthority())
    {
        return;
    }

    // CombatState 리셋
    if (USFPlayerCombatStateComponent* CombatComp = PlayerState->FindComponentByClass<USFPlayerCombatStateComponent>())
    {
        CombatComp->ResetDownCount();
        CombatComp->SetIsDead(false);
    }

	APlayerController* PC = PlayerState->GetPlayerController();
	if (!PC)
	{
		return;
	}

	// SpectatorPawn 정리 (RestartPlayer가 새 Pawn을 스폰하도록)
	if (APawn* CurrentPawn = PC->GetPawn())
	{
		PC->UnPossess();
		CurrentPawn->Destroy();
	}

	// 이제 GetPawn() == nullptr이므로 새 Pawn 스폰됨
	RestartPlayer(PC);
	
    if (APawn* NewPawn = PC->GetPawn())
    {
        if (USFPawnExtensionComponent* PawnExtComp = USFPawnExtensionComponent::FindPawnExtensionComponent(NewPawn))
        {
            // weak pointer로 캡처하여 prevent dangling
            TWeakObjectPtr<ASFPlayerState> WeakPS = PlayerState;
            PawnExtComp->OnAbilitySystemInitialized_RegisterAndCall
            (FSimpleMulticastDelegate::FDelegate::CreateWeakLambda(this,[this, WeakPS]()
            {
                if (ASFPlayerState* PS = WeakPS.Get())
                {
                    OnResurrectionReady(PS);
                }
            }));
        }
    }
    
}

void ASFGameMode::ReviveDownedPlayer(ASFPlayerState* PlayerState)
{
	if (!PlayerState || !HasAuthority())
	{
		return;
	}

	// CombatState 리셋 (DownCount 복구)
	if (USFPlayerCombatStateComponent* CombatComp = PlayerState->FindComponentByClass<USFPlayerCombatStateComponent>())
	{
		CombatComp->ResetDownCount();
	}

	// Resurrection 어빌리티 즉시 활성화 (Downed 캔슬 + 회복)
	if (USFAbilitySystemComponent* ASC = PlayerState->GetSFAbilitySystemComponent())
	{
		if (ResurrectionAbilityClass)
		{
			ASC->TryActivateAbilityByClass(ResurrectionAbilityClass);
		}
	}
}

void ASFGameMode::OnResurrectionReady(ASFPlayerState* PlayerState)
{
    if (!PlayerState)
    {
        return;
    }

    USFAbilitySystemComponent* ASC = PlayerState->GetSFAbilitySystemComponent();
    if (!ASC || !ResurrectionAbilityClass)
    {
        return;
    }

    UE_LOG(LogSF, Warning, TEXT("[GameMode] OnResurrectionReady: %s"), *PlayerState->GetPlayerName());
	
    // 부활 어빌리티 활성화 
    ASC->TryActivateAbilityByClass(ResurrectionAbilityClass);
}

bool ASFGameMode::IsBossStage() const
{
	if (ASFGameState* SFGS = GetGameState<ASFGameState>())
	{
		if (USFStageManagerComponent* StageManager = SFGS->GetStageManager())
		{
			if (bUsePIETestResurrectionInAnyStage)
			{
				return true;
			}
			return StageManager->GetCurrentStageInfo().IsBossStage();
		}
	}
	return false;
}

bool ASFGameMode::IsFinalStage() const
{
	if (ASFGameState* SFGS = GetGameState<ASFGameState>())
	{
		if (USFStageManagerComponent* StageManager = SFGS->GetStageManager())
		{
			return StageManager->GetCurrentStageInfo().bIsFinalStage;
		}
	}
	return false;
}

void ASFGameMode::OnGameOver()
{
	// TODO : AI 비활성화, 게임플레이 시스템 정지, 서버 전용 정리 작업
}
