#include "SFPlayerController.h"

#include "AbilitySystemGlobals.h"
#include "SFPlayerState.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Components/SFLoadingCheckComponent.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/SFDeathUIComponent.h"
#include "Components/SFSharedUIComponent.h"
#include "Components/SFSpectatorComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Pawn/SFSpectatorPawn.h"
#include "System/SFPlayFabSubsystem.h"
#include "UI/InGame/SFIndicatorWidgetBase.h"

ASFPlayerController::ASFPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LoadingCheckComponent = CreateDefaultSubobject<USFLoadingCheckComponent>(TEXT("LoadingCheckComponent"));
	SpectatorComponent = CreateDefaultSubobject<USFSpectatorComponent>(TEXT("SpectatorComponent"));
	DeathUIComponent = CreateDefaultSubobject<USFDeathUIComponent>(TEXT("DeathUIComponent"));
	SharedUIComponent = CreateDefaultSubobject<USFSharedUIComponent>(TEXT("SharedUIComponent"));
}

void ASFPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

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

	// 기존 InputComponent를 향상된 버전(EnhancedInputComponent)으로 Cast
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (InGameMenuAction)
		{
			// BindAction: 이 액션이 시작되면(Started) -> ToggleInGameMenu 함수를 실행
			// ETriggerEvent::Started는 "키를 누르는 순간"
			EnhancedInputComponent->BindAction(InGameMenuAction, ETriggerEvent::Started, this, &ASFPlayerController::ToggleInGameMenu);
		}
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

void ASFPlayerController::ToggleInGameMenu()
{
	UE_LOG(LogTemp, Warning, TEXT("ESC 키 눌림! ToggleInGameMenu 함수 진입 성공"));
	
	// 1. 이미 메뉴가 켜져 있다면? -> 종료
	if (InGameMenuInstance && InGameMenuInstance->IsInViewport())
	{
		InGameMenuInstance->RemoveFromParent();
		InGameMenuInstance = nullptr;

		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
		return;
	}

	// 2. 메뉴가 꺼져 있다면? -> 실행
	if (InGameMenuClass)
	{
		InGameMenuInstance = CreateWidget<UUserWidget>(this, InGameMenuClass);

		if (InGameMenuInstance)
		{
			// Z-Order 100으로 최상단 배치
			InGameMenuInstance->AddToViewport(100);

			// 위젯 자체를 포커스 타겟으로 명확히 지정
			FInputModeUIOnly InputMode;
			// 위젯이 포커스를 받을 수 있게 설정 
			InputMode.SetWidgetToFocus(InGameMenuInstance->TakeWidget());
			// 마우스가 화면 밖으로 나가지 않게 잠금
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			SetInputMode(InputMode);
			bShowMouseCursor = true;

			InGameMenuInstance->SetKeyboardFocus();
		}
	}
	
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
