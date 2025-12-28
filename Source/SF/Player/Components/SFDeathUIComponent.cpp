#include "SFDeathUIComponent.h"

#include "SFSpectatorComponent.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Character/Hero/SFHero.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/GameStateBase.h"
#include "Messages/SFMessageGameplayTags.h"
#include "Player/SFPlayerState.h"
#include "System/SFInitGameplayTags.h"
#include "UI/InGame/GameOverScreenWidget.h"
#include "UI/InGame/SFDeathScreenWidget.h"

const FName USFDeathUIComponent::NAME_DeathUIFeature("DeathUI");

USFDeathUIComponent::USFDeathUIComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void USFDeathUIComponent::OnRegister()
{
	Super::OnRegister();
	RegisterInitStateFeature();
}

void USFDeathUIComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetController<APlayerController>();
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	if (UGameplayMessageSubsystem::HasInstance(this))
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		GameOverListenerHandle = MessageSubsystem.RegisterListener(SFGameplayTags::Message_Game_GameOver,this, &ThisClass::OnGameOver);
	}

	// 초기 상태(Spawned) 진입 시도
	ensure(TryToChangeInitState(SFGameplayTags::InitState_Spawned));
    
	// 초기화 체인 시작
	CheckDefaultInitialization();
}

void USFDeathUIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameplayMessageSubsystem::HasInstance(this))
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(GameOverListenerHandle);
	}
	
	if (USFPlayerCombatStateComponent* CombatComp = CachedCombatComp.Get())
	{
		CombatComp->OnCombatInfoChanged.RemoveDynamic(this, &ThisClass::OnCombatInfoChanged);
	}
	
	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}

void USFDeathUIComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = { SFGameplayTags::InitState_Spawned, SFGameplayTags::InitState_DataAvailable, SFGameplayTags::InitState_DataInitialized, SFGameplayTags::InitState_GameplayReady };

	ContinueInitStateChain(StateChain);
}

bool USFDeathUIComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APlayerController* PC = GetController<APlayerController>();
	if (!PC)
	{
		return false;
	}

	// [None -> Spawned]
	if (!CurrentState.IsValid() && DesiredState == SFGameplayTags::InitState_Spawned)
	{
		return true;
	}

	// [Spawned -> DataAvailable]: PlayerState + CombatInfo 복제 완료 확인
	if (CurrentState == SFGameplayTags::InitState_Spawned && DesiredState == SFGameplayTags::InitState_DataAvailable)
	{
		ASFPlayerState* PS = PC->GetPlayerState<ASFPlayerState>();
		if (!PS)
		{
			return false;
		}

		USFPlayerCombatStateComponent* CombatComp = PS->FindComponentByClass<USFPlayerCombatStateComponent>();
		if (!CombatComp)
		{
			return false;
		}

		//  데이터가 아직 안 왔다면 콜백 등록 후 대기
		if (!CombatComp->HasReceivedInitialCombatInfo())
		{
			USFDeathUIComponent* MutableThis = const_cast<USFDeathUIComponent*>(this);
			if (!CombatComp->OnCombatInfoChanged.IsAlreadyBound(MutableThis, &ThisClass::OnInitialCombatInfoReceived))
			{
				CombatComp->OnCombatInfoChanged.AddDynamic(MutableThis, &ThisClass::OnInitialCombatInfoReceived);
			}
			return false;
		}

		return true;
	}

	// [DataAvailable -> DataInitialized]
	if (CurrentState == SFGameplayTags::InitState_DataAvailable && DesiredState == SFGameplayTags::InitState_DataInitialized)
	{
		return true;
	}

	// [DataInitialized -> GameplayReady]
	if (CurrentState == SFGameplayTags::InitState_DataInitialized && DesiredState == SFGameplayTags::InitState_GameplayReady)
	{
		return true;
	}

	return false;
}

void USFDeathUIComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (CurrentState == SFGameplayTags::InitState_DataAvailable && DesiredState == SFGameplayTags::InitState_DataInitialized)
	{
		APlayerController* PC = GetController<APlayerController>();
		ASFPlayerState* PS = PC ? PC->GetPlayerState<ASFPlayerState>() : nullptr;
        
		if (PS)
		{
			USFPlayerCombatStateComponent* CombatComp = PS->GetComponentByClass<USFPlayerCombatStateComponent>();
			InitializeDeathSystem(CombatComp);
		}
	}
}

void USFDeathUIComponent::OnInitialCombatInfoReceived(const FSFHeroCombatInfo& CombatInfo)
{
	// 임시 바인딩 해제
	if (APlayerController* PC = GetController<APlayerController>())
	{
		if (ASFPlayerState* PS = PC->GetPlayerState<ASFPlayerState>())
		{
			if (USFPlayerCombatStateComponent* CombatComp = PS->FindComponentByClass<USFPlayerCombatStateComponent>())
			{
				CombatComp->OnCombatInfoChanged.RemoveDynamic(this, &ThisClass::OnInitialCombatInfoReceived);
			}
		}
	}

	// InitState 재시도 (이제 HasReceivedInitialCombatInfo가 true)
	CheckDefaultInitialization();
}

void USFDeathUIComponent::InitializeDeathSystem(USFPlayerCombatStateComponent* CombatComp)
{
	if (!CombatComp)
	{
		return;
	}

	CachedCombatComp = CombatComp;

	// 이벤트 바인딩
	if (!CombatComp->OnCombatInfoChanged.IsAlreadyBound(this, &ThisClass::OnCombatInfoChanged))
	{
		CombatComp->OnCombatInfoChanged.AddDynamic(this, &ThisClass::OnCombatInfoChanged);
	}

	// 초기 상태 기록 (중복 호출 방지)
	bLastKnownDeadState = CombatComp->IsDead();

	// Seamless Travel 복구: 이미 죽어있으면 바로 관전 모드 (DeathScreen 스킵)
	if (CombatComp->IsDead())
	{
		APlayerController* PC = GetController<APlayerController>();
		if (PC)
		{
			if (USFSpectatorComponent* SpectatorComp = PC->FindComponentByClass<USFSpectatorComponent>())
			{
				SpectatorComp->StartSpectating();
			}
		}
	}
}

void USFDeathUIComponent::OnCombatInfoChanged(const FSFHeroCombatInfo& CombatInfo)
{
	// 상태 변화 없으면 무시 (중복 호출 방지)
	if (CombatInfo.bIsDead == bLastKnownDeadState)
	{
		return;
	}

	bLastKnownDeadState = CombatInfo.bIsDead;

	if (CombatInfo.bIsDead)
	{
		// 게임 플레이 중 사망 → DeathScreen 표시
		ShowDeathScreen();
	}
	else
	{
		// 부활
		OnResurrected();
	}
}

void USFDeathUIComponent::ShowDeathScreen()
{
	if (bIsGameOver)
	{
		return; // GameOver 이벤트가 곧 오므로 스킵
	}

	if (AreAllOtherPlayersIncapacitated())
	{
		return; // GameOver 이벤트가 곧 오므로 스킵
	}
	
	APlayerController* PC = GetController<APlayerController>();
	if (!PC || !DeathScreenWidgetClass)
	{
		return;
	}

	if (!DeathScreenWidget)
	{
		DeathScreenWidget = CreateWidget<USFDeathScreenWidget>(PC, DeathScreenWidgetClass);
		DeathScreenWidget->OnDeathAnimationFinished.AddDynamic(this, &ThisClass::OnDeathAnimationFinished);
	}

	if (DeathScreenWidget && !DeathScreenWidget->IsInViewport())
	{
		DeathScreenWidget->AddToViewport(DeathScreenZOrder);
		DeathScreenWidget->PlayDeathDirection();
	}
}

void USFDeathUIComponent::OnResurrected()
{
	APlayerController* PC = GetController<APlayerController>();
	if (!PC)
	{
		return;
	}

	// DeathScreen만 제거
	if (DeathScreenWidget && DeathScreenWidget->IsInViewport())
	{
		DeathScreenWidget->RemoveFromParent();
	}

	// 새 Pawn의 InitState 바인딩
	BindToNewPawnInitState();
}

void USFDeathUIComponent::BindToNewPawnInitState()
{
	APlayerController* PC = GetController<APlayerController>();
	if (!PC)
	{
		return;
	}

	APawn* CurrentPawn = PC->GetPawn();
    
	// 아직 새 Pawn이 없거나 SpectatorPawn이면 OnPossessedPawnChanged로 대기
	if (!CurrentPawn || !CurrentPawn->IsA(ASFHero::StaticClass()))
	{
		// Pawn 변경 시 다시 시도
		if (!PC->OnPossessedPawnChanged.IsAlreadyBound(this, &ThisClass::OnPossessedPawnChanged))
		{
			PC->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::OnPossessedPawnChanged);
		}
		return;
	}

	// 새 Hero Pawn이 있으면 InitState 바인딩
	if (UGameFrameworkComponentManager* Manager = UGameFrameworkComponentManager::GetForActor(CurrentPawn))
	{
		// 이미 GameplayReady면 즉시 콜백, 아니면 도달 시 콜백
		PawnInitStateHandle = Manager->RegisterAndCallForActorInitState(CurrentPawn, USFPawnExtensionComponent::NAME_ActorFeatureName, SFGameplayTags::InitState_GameplayReady,
			FActorInitStateChangedDelegate::CreateUObject(this, &ThisClass::OnNewPawnGameplayReady));
	}
}

void USFDeathUIComponent::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	APlayerController* PC = GetController<APlayerController>();
	if (!PC)
	{
		return;
	}

	// 새 Hero Pawn이면 InitState 바인딩
	if (NewPawn && NewPawn->IsA(ASFHero::StaticClass()))
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::OnPossessedPawnChanged);
		BindToNewPawnInitState();
	}
}

void USFDeathUIComponent::OnNewPawnGameplayReady(const FActorInitStateChangedParams& Params)
{
	// 핸들 초기화
	PawnInitStateHandle.Reset();

	APlayerController* PC = GetController<APlayerController>();
	if (!PC)
	{
		return;
	}

	// 관전 모드 종료
	if (USFSpectatorComponent* SpectatorComp = PC->FindComponentByClass<USFSpectatorComponent>())
	{
		SpectatorComp->StopSpectating();
	}
}

void USFDeathUIComponent::OnDeathAnimationFinished()
{
	if (bIsGameOver)
	{
		return; // GameOver 위젯을 스킵했으므로(애니메이션 출력중이라) 스크린 유지 필요
	}
	
	if (DeathScreenWidget && DeathScreenWidget->IsInViewport())
	{
		DeathScreenWidget->RemoveFromParent();
	}
	
	if (USFPlayerCombatStateComponent* CombatComp = CachedCombatComp.Get())
	{
		if (!CombatComp->IsDead())
		{
			return; 
		}
	}

	// 관전 모드 시작
	APlayerController* PC = GetController<APlayerController>();
	if (PC)
	{
		if (USFSpectatorComponent* SpectatorComp = PC->FindComponentByClass<USFSpectatorComponent>())
		{
			SpectatorComp->StartSpectating();
		}
	}
}

void USFDeathUIComponent::OnGameOver(FGameplayTag Channel, const FSFGameOverMessage& Message)
{
	bIsGameOver = true;
	
	ShowGameOverScreen();
}

void USFDeathUIComponent::ShowGameOverScreen()
{
	APlayerController* PC = GetController<APlayerController>();
	if (!PC || !GameOverWidgetClass)
	{
		return;
	}

	// DeathScreen이 이미 표시 중이면 유지
	if (DeathScreenWidget && DeathScreenWidget->IsInViewport())
	{
		return;
	}

	// GameOver 화면 표시 (관전 중이던 플레이어들)
	if (!GameOverWidget)
	{
		GameOverWidget = CreateWidget<UGameOverScreenWidget>(PC, GameOverWidgetClass);
	}

	if (GameOverWidget && !GameOverWidget->IsInViewport())
	{
		GameOverWidget->AddToViewport(GameOverScreenZOrder);
		GameOverWidget->PlayGameOverDirection();
	}
}

bool USFDeathUIComponent::AreAllOtherPlayersIncapacitated()
{
	AGameStateBase* GameState = GetWorld()->GetGameState<AGameStateBase>();
	if (!GameState)
	{
		return false;
	}

	APlayerController* MyPC = GetController<APlayerController>();
	if (!MyPC)
	{
		return false;
	}

	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (PS->IsInactive() || PS == MyPC->PlayerState)
		{
			continue;
		}

		const USFPlayerCombatStateComponent* CombatComp = PS->FindComponentByClass<USFPlayerCombatStateComponent>();
		if (CombatComp && !CombatComp->IsIncapacitated())
		{
			return false; // 다른 생존자 있음
		}
	}

	return true; // 나만 남음
}
