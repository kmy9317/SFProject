#include "SFPlayerController.h"

#include "AbilitySystemGlobals.h"
#include "SFPlayerState.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Components/SFLoadingCheckComponent.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputComponent.h"
#include "Character/Enemy/Component/SFEnemyWidgetComponent.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/SFDeathUIComponent.h"
#include "Components/SFSharedUIComponent.h"
#include "Components/SFSpectatorComponent.h"
#include "UI/Compoent/SFInGameMenuComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameModes/SFGameOverManagerComponent.h"
#include "Inventory/SFInventoryManagerComponent.h"
#include "Inventory/SFQuickbarComponent.h"
#include "Item/SFItemManagerComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameModes/SFGameState.h"
#include "GameModes/SFStageManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Pawn/SFSpectatorPawn.h"
#include "System/SFPlayFabSubsystem.h"
#include "UI/InGame/SFBossHUDWidget.h"
#include "UI/InGame/SFIndicatorWidgetBase.h"
#include "UI/InGame/SFDamageWidget.h"

ASFPlayerController::ASFPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LoadingCheckComponent = CreateDefaultSubobject<USFLoadingCheckComponent>(TEXT("LoadingCheckComponent"));
	SpectatorComponent = CreateDefaultSubobject<USFSpectatorComponent>(TEXT("SpectatorComponent"));
	DeathUIComponent = CreateDefaultSubobject<USFDeathUIComponent>(TEXT("DeathUIComponent"));
	SharedUIComponent = CreateDefaultSubobject<USFSharedUIComponent>(TEXT("SharedUIComponent"));
	InventoryManagerComponent = CreateDefaultSubobject<USFInventoryManagerComponent>(TEXT("InventoryManagerComponent"));
	QuickbarComponent = CreateDefaultSubobject<USFQuickbarComponent>(TEXT("QuickbarComponent"));
	ItemManagerComponent = CreateDefaultSubobject<USFItemManagerComponent>(TEXT("ItemManagerComponent"));
	InGameMenuComponent = CreateDefaultSubobject<USFInGameMenuComponent>(TEXT("SystemMenuComponent"));
}

void ASFPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);

	// 몬스터 데미지 텍스트 관련 리스너 등록 (UI.Event.Damage) -> OnDamageMessageReceived() 실행
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	
	DamageMessageListenerHandle = MessageSubsystem.RegisterListener(
		FGameplayTag::RequestGameplayTag("UI.Event.Damage"), 
		this, 
		&ThisClass::OnDamageMessageReceived
	);

	// 로컬 플레이어인 경우 팀원 표시 로직 실행
	if (IsLocalController())
	{
		GetWorld()->GetTimerManager().SetTimer(
			TeammateSearchTimerHandle,
			this,
			&ASFPlayerController::CreateTeammateIndicators,
			1.0f,
			true
			);
	}
}

void ASFPlayerController::OnUnPossess()
{
	// Make sure the pawn that is being unpossessed doesn't remain our ASC's avatar actor
	if (APawn* PawnBeingUnpossessed = GetPawn())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState))
		{
			if (ASC->GetAvatarActor() == PawnBeingUnpossessed)
			{
				ASC->SetAvatarActor(nullptr);
			}
		}
	}

	Super::OnUnPossess();
}

void ASFPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 클라이언트에서 PlayerController가 늦게 복제된 경우
	if (GetWorld()->IsNetMode(NM_Client))
	{
		if (ASFPlayerState* SFPS = GetPlayerState<ASFPlayerState>())
		{
			if (USFAbilitySystemComponent* SFASC = SFPS->GetSFAbilitySystemComponent())
			{
				SFASC->RefreshAbilityActorInfo();
				SFASC->TryActivateAbilitiesOnSpawn();
			}
		}
	}

	// PlayerState가 도착했으므로 이를 기다리던 컴포넌트들의 초기화 재시도
	for (UActorComponent* Comp : GetComponents())
	{
		if (IGameFrameworkInitStateInterface* InitInterface = Cast<IGameFrameworkInitStateInterface>(Comp))
		{
			InitInterface->CheckDefaultInitialization();
		}
	}
}

void ASFPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		InGameMenuComponent->SetupInputBindings(EnhancedInputComponent);
	}
}

void ASFPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	ASFPlayerState* SFPS = GetPlayerState<ASFPlayerState>();
	APawn* CurrentPawn = GetPawn();

	// 관전 중(SpectatorPawn)일 때는 내 시선을 공유할 필요 없음 
	if (CurrentPawn && CurrentPawn->IsA(ASFSpectatorPawn::StaticClass()))
	{
		return; 
	}

	// 일반 캐릭터 조종 중
	if (SFPS && IsLocalController())
	{
		FRotator MyViewRotation = GetControlRotation();
		
		// [Client Local] 로컬 예측을 위해 내 변수 즉시 업데이트 (반응성)
		SFPS->SetReplicatedViewRotation(MyViewRotation);

		// 서버 전송 최적화: 변경 감지 + 빈도 제한
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		const bool bRotationChanged = !MyViewRotation.Equals(LastSentViewRotation, ViewRotationThreshold);
		const bool bEnoughTimePassed = (CurrentTime - LastViewRotationSendTime) >= ViewRotationSendInterval;
		if (bRotationChanged && bEnoughTimePassed)
		{
			Server_UpdateViewRotation(MyViewRotation);
			LastSentViewRotation = MyViewRotation;
			LastViewRotationSendTime = CurrentTime;
		}
	}
}

void ASFPlayerController::Server_UpdateViewRotation_Implementation(FRotator NewRotation)
{
	ASFPlayerState* SFPS = GetPlayerState<ASFPlayerState>();
	if (SFPS)
	{
		SFPS->SetReplicatedViewRotation(NewRotation);
	}
}

ASFPlayerState* ASFPlayerController::GetSFPlayerState() const
{
	return CastChecked<ASFPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

USFAbilitySystemComponent* ASFPlayerController::GetSFAbilitySystemComponent() const
{
	const ASFPlayerState* LCPS = GetSFPlayerState();
	return (LCPS ? LCPS->GetSFAbilitySystemComponent() : nullptr);
}

void ASFPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (USFAbilitySystemComponent* SFASC = GetSFAbilitySystemComponent())
	{
		SFASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}
	
	Super::PostProcessInput(DeltaTime, bGamePaused);

}

void ASFPlayerController::CreateTeammateIndicators()
{
	if (!TeammateIndicatorWidgetClass) 
	{
		return;
	}

	// 1. 죽거나 사라진 팀원 위젯 청소 (Cleanup)
	for (auto It = TeammateWidgetMap.CreateIterator(); It; ++It)
	{
		AActor* KeyActor = It.Key();
		USFIndicatorWidgetBase* Widget = It.Value();

		// 조건: 액터가 파괴되었거나(IsValid 실패), 위젯이 타겟을 잃어버렸다면
		if (!IsValid(KeyActor) || (Widget && !Widget->HasValidTarget()))
		{
			if (Widget)
			{
				Widget->RemoveFromParent(); // 화면에서 즉시 지움
			}
			It.RemoveCurrent();
		}
	}

	// 2. 새로운 팀원 검색 및 위젯 생성

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), FoundActors);

	APawn* MyPawn = GetPawn();

	for (AActor* Actor : FoundActors)
	{
		// A. 플레이어 자신이면 패스
		if (Actor == MyPawn) continue;

		// B. 이미 위젯을 생성한 액터면 패스
		if (TeammateWidgetMap.Contains(Actor)) continue;

		// C. 진짜 플레이어인지 확인 (몬스터 제외)
		APawn* TargetPawn = Cast<APawn>(Actor);
		if (!TargetPawn) continue;

		// PlayerState가 없으면 봇이나 몬스터로 간주하고 패스 -> 혹시 PlayerState가 늦게 로딩될 수도 있으니, 이번 틱에 없으면 다음 틱에 재검사
		if (TargetPawn->GetPlayerState() == nullptr) continue;

		// === [조건 만족: 위젯 생성] ===
		USFIndicatorWidgetBase* NewIndicator = CreateWidget<USFIndicatorWidgetBase>(this, TeammateIndicatorWidgetClass);
		if (NewIndicator)
		{
			// 타겟 설정
			NewIndicator->SetTargetActor(Actor);
			
			// 화면 최하단(-1)에 부착 -> 다른 HUD를 가리지 않도록
			NewIndicator->AddToViewport(-1);
			
			// 관리 맵에 등록
			TeammateWidgetMap.Add(Actor, NewIndicator);
			
			UE_LOG(LogTemp, Log, TEXT("Team Indicator Created for: %s"), *Actor->GetName());
		}
	}
}

void ASFPlayerController::OnDamageMessageReceived(FGameplayTag Channel, const FSFDamageMessageInfo& Payload)
{
	// 유효성 체크 -> 타겟 확인
	if (!Payload.TargetActor) return;

	APawn* MyPawn = GetPawn();
	if (!MyPawn) return;
	APlayerState* MyPS = GetPlayerState<APlayerState>();

	/*UE_LOG(LogTemp, Warning, TEXT("Damage Debug: Instigator=[%s], MyPawn=[%s], MyController=[%s]"),
	Payload.Instigator ? *Payload.Instigator->GetName() : TEXT("NULL"),
	*MyPawn->GetName(),
	*GetName());
	*/

	bool bIsMyPawn = (Payload.Instigator == MyPawn);
	bool bIsMyController = (Payload.Instigator == this);
	bool bIsMyPlayerState = (MyPS && Payload.Instigator == MyPS);
	
	if (!bIsMyPawn && !bIsMyController && !bIsMyPlayerState)
	{
		return; 
	}
	
	Client_ShowDamageText(Payload.DamageAmount, Payload.TargetActor);
}

void ASFPlayerController::Client_ShowDamageText_Implementation(float DamageAmount, AActor* TargetActor)
{
	// 1. 유효성 체크
	if (!DamageWidgetClass || !TargetActor) return;

	// 플레이어/아군 체크
	APawn* TargetPawn = Cast<APawn>(TargetActor);
	if (!TargetPawn) return;
	if (TargetPawn->IsPlayerControlled()) return; 
	if (TargetPawn->ActorHasTag(FName("Player"))) return;

	FVector TextSpawnLocation = TargetActor->GetActorLocation();

	USFEnemyWidgetComponent* HPBarComp = TargetActor->FindComponentByClass<USFEnemyWidgetComponent>();

	if (HPBarComp)
	{
		TextSpawnLocation = HPBarComp->GetComponentLocation();
	}
	else
	{
		ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
		if (TargetCharacter)
		{
			TextSpawnLocation.Z += TargetCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 50.0f;
		}
	}

	if (PlayerCameraManager)
	{
		const FTransform CamTransform = PlayerCameraManager->GetTransform();
		const FVector CamRight = CamTransform.GetUnitAxis(EAxis::Y);
		const FVector CamUp = CamTransform.GetUnitAxis(EAxis::Z);

		// 오른쪽 50, 위 40
		TextSpawnLocation += (CamRight * 50.0f) + (CamUp * 40.0f);
	}

	USFDamageWidget* NewDamageWidget = CreateWidget<USFDamageWidget>(this, DamageWidgetClass);
	if (NewDamageWidget)
	{
		NewDamageWidget->AddToViewport();
	}
}

void ASFPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 1. 팀원 검색 타이머 강제 종료
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TeammateSearchTimerHandle);
	}

	// 2. GameInstance 가져오기 
	UGameInstance* GI = GetGameInstance();

	// 만약 PC가 이미 연결이 끊겨서 GI를 가져올 수 없다면
	if (!GI && GetWorld())
	{
		// 월드를 통해 강제로 GameInstance를 찾아옴.
		GI = GetWorld()->GetGameInstance();
	}

	// GI를 찾았고, 시스템이 살아있다면
	if (GI)
	{
		if (UGameplayMessageSubsystem* MessageSubsystem = GI->GetSubsystem<UGameplayMessageSubsystem>())
		{
			// 리스너 해제 (Unregister)
			MessageSubsystem->UnregisterListener(DamageMessageListenerHandle);
		}
	}
	
	// 3. 팀원 표시 위젯 정리 (Viewport 참조 해제)
	for (auto& Elem : TeammateWidgetMap)
	{
		if (Elem.Value)
		{
			// 위젯이 유효하다면 부모(Viewport)에서 분리
			Elem.Value->RemoveFromParent();
		}
	}
	TeammateWidgetMap.Empty();
	
	Super::EndPlay(EndPlayReason);
}

void ASFPlayerController::Server_SendPermanentUpgradeData_Implementation(const FSFPermanentUpgradeData& InData)
{
	ASFPlayerState* PS = GetPlayerState<ASFPlayerState>();
	if (!PS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PermanentUpgrade] Server_SendPermanentUpgradeData: PS null"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[PermanentUpgrade] Server received data from PC. Wrath=%d Pride=%d Lust=%d Sloth=%d Greed=%d"),
		InData.Wrath, InData.Pride, InData.Lust, InData.Sloth, InData.Greed);

	PS->SetPermanentUpgradeData(InData);   // 여기서 bReceived=true & TryApply로 이어지게
}

// SFPlayerController.cpp
void ASFPlayerController::Client_BeginPermanentUpgradeFlow_Implementation()
{
	if (!IsLocalController())
	{
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (USFPlayFabSubsystem* PF = GI->GetSubsystem<USFPlayFabSubsystem>())
		{
			UE_LOG(LogTemp, Warning, TEXT("[PermanentUpgrade][Client] Begin flow"));

			PF->ResetPermanentUpgradeSendState();
			PF->StartRetrySendPermanentUpgradeDataToServer();
		}
	}
}

void ASFPlayerController::RequestReadyForLobby()
{
	Server_NotifyReadyForLobby();
}

void ASFPlayerController::CreateBossHUD()
{
	ASFGameState* GS = GetWorld()->GetGameState<ASFGameState>();
	if (GS)
	{
		if (GS->GetStageManager() && GS->GetStageManager()->GetCurrentBossActor())
		{
			if (BossHUDWidgetClass && !BossHUDWidgetInstance)
			{
					BossHUDWidgetInstance = CreateWidget<USFBossHUDWidget>(this, BossHUDWidgetClass);
				if (BossHUDWidgetInstance)
				{
					BossHUDWidgetInstance->AddToViewport(30);
					BossHUDWidgetInstance->InitializeBoss(GS->GetStageManager()->GetCurrentBossActor());
					GS->GetStageManager()->OnBossStateChanged.AddDynamic(this, &ThisClass::RemoveBossHUD);
				}
			}
		}
	}
}

void ASFPlayerController::RemoveBossHUD(ACharacter* BossActor)
{
		if (!IsValid(BossActor))
		{
			if (BossHUDWidgetInstance)
			{
				BossHUDWidgetInstance->RemoveFromParent();
				BossHUDWidgetInstance = nullptr;
			}

			ASFGameState* GS = GetWorld()->GetGameState<ASFGameState>();
			if (GS)
			{
				if (GS->GetStageManager())
				{
					GS->GetStageManager()->OnBossStateChanged.RemoveDynamic(this, &ThisClass::RemoveBossHUD);
				}
			}
		}
} 

void ASFPlayerController::Server_NotifyReadyForLobby_Implementation()
{
	if (UWorld* World = GetWorld())
	{
		if (AGameStateBase* GameState = World->GetGameState<AGameStateBase>())
		{
			if (USFGameOverManagerComponent* GameOverManager = GameState->FindComponentByClass<USFGameOverManagerComponent>())
			{
				GameOverManager->NotifyPlayerReadyForLobby(this);
			}
		}
	}
}
