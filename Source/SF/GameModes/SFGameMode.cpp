#include "SFGameMode.h"

#include "EngineUtils.h"
#include "SFEnemyManagerComponent.h"
#include "SFGameState.h"
#include "SFLogChannels.h"
#include "SFPortalManagerComponent.h"
#include "Player/SFPlayerInfoTypes.h"
#include "Player/SFPlayerState.h"
#include "Character/Hero/SFHeroDefinition.h"
#include "Character/SFPawnData.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Engine/PlayerStartPIE.h"
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

void ASFGameMode::RequestTravelToNextStage(TSoftObjectPtr<UWorld> NextStageLevel)
{
	if (!HasAuthority())
	{
		return;
	}

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

	UE_LOG(LogSF, Log, TEXT("[GameMode] Traveling to next stage: %s"), *NextStageLevel.ToString());

	if (USFGameInstance* SFGameInstance = Cast<USFGameInstance>(GetWorld()->GetGameInstance()))
	{
		SFGameInstance->LoadLevelAndListen(NextStageLevel);
	}
}

void ASFGameMode::OnAllEnemiesDefeated()
{
	UE_LOG(LogSF, Warning, TEXT("[GameMode] Stage cleared! Activating portal..."));

	ActivatePortal();
}
