#include "SFGameMode.h"

#include "SFGameState.h"
#include "SFLogChannels.h"
#include "SFPortalManagerComponent.h"
#include "Player/SFPlayerInfoTypes.h"
#include "Player/SFPlayerState.h"
#include "Character/Hero/SFHeroDefinition.h"
#include "Character/SFPawnData.h"
#include "Character/SFPawnExtensionComponent.h"
#include "System/SFGameInstance.h"

ASFGameMode::ASFGameMode()
{
	bUseSeamlessTravel = true;
}

void ASFGameMode::InitGameState()
{
	Super::InitGameState();
	// TODO : SFGameState 캐싱
}

void ASFGameMode::StartPlay()
{
	Super::StartPlay();

	// TODO : 테스트용 자동 포탈 활성화(삭제 예정)
	if (bAutoActivatePortal)
	{
		GetWorld()->GetTimerManager().SetTimer(
			PortalActivationTimerHandle,
			this,
			&ASFGameMode::AutoActivatePortalForTest,
			PortalActivationDelay,
			false
		);

		UE_LOG(LogSF, Warning, TEXT("[GameMode] Portal will activate in %.1f seconds"), PortalActivationDelay);
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

void ASFGameMode::AutoActivatePortalForTest()
{
	//ActivatePortal();
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

	UE_LOG(LogSF, Log, TEXT("[GameMode] Traveling to next stage: %s"), *NextStageLevel.ToString());

	if (USFGameInstance* SFGameInstance = Cast<USFGameInstance>(GetWorld()->GetGameInstance()))
	{
		SFGameInstance->LoadLevelAndListen(NextStageLevel);
	}
}
