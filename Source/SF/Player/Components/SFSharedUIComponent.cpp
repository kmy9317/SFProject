#include "SFSharedUIComponent.h"

#include "Components/GameFrameworkComponentManager.h"
#include "Player/SFPlayerState.h"
#include "Player/Components/SFPlayerCombatStateComponent.h"
#include "System/SFInitGameplayTags.h"
#include "UI/SFHUD.h"
#include "GameFramework/GameStateBase.h"

const FName USFSharedUIComponent::NAME_SharedUIFeature("SharedUI");

USFSharedUIComponent::USFSharedUIComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = false;
    bWantsInitializeComponent = true;
}

void USFSharedUIComponent::OnRegister()
{
    Super::OnRegister();
    RegisterInitStateFeature();
}

void USFSharedUIComponent::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC = GetController<APlayerController>();
    if (!PC || !PC->IsLocalController())
    {
        return;
    }

    ensure(TryToChangeInitState(SFGameplayTags::InitState_Spawned));
    CheckDefaultInitialization();
}

void USFSharedUIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}

void USFSharedUIComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = {SFGameplayTags::InitState_Spawned,SFGameplayTags::InitState_DataAvailable,SFGameplayTags::InitState_DataInitialized,SFGameplayTags::InitState_GameplayReady};
	ContinueInitStateChain(StateChain);
}

bool USFSharedUIComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APlayerController* PC = GetController<APlayerController>();
	if (!PC)
	{
		return false;
	}

	// [None -> Spawned]: PC 존재하면 통과
	if (!CurrentState.IsValid() && DesiredState == SFGameplayTags::InitState_Spawned)
	{
		return true;
	}

	// [Spawned -> DataAvailable]: PlayerState 확인
	if (CurrentState == SFGameplayTags::InitState_Spawned && DesiredState == SFGameplayTags::InitState_DataAvailable)
	{
		ASFPlayerState* PS = PC->GetPlayerState<ASFPlayerState>();
		if (!PS)
		{
			return false;
		}

		return true;
	}

	// [DataAvailable -> DataInitialized]: 항상 통과
	if (CurrentState == SFGameplayTags::InitState_DataAvailable && DesiredState == SFGameplayTags::InitState_DataInitialized)
	{
		return true;
	}

	// [DataInitialized -> GameplayReady]: HUD 존재 확인
	if (CurrentState == SFGameplayTags::InitState_DataInitialized && DesiredState == SFGameplayTags::InitState_GameplayReady)
	{
		ASFHUD* HUD = PC->GetHUD<ASFHUD>();
		return HUD != nullptr;
	}

	return false;
}

void USFSharedUIComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	APlayerController* PC = GetController<APlayerController>();

	// [DataInitialized -> GameplayReady]: HUD에 SharedOverlay 초기화 요청
	if (CurrentState == SFGameplayTags::InitState_DataInitialized && DesiredState == SFGameplayTags::InitState_GameplayReady)
	{
		if (ASFHUD* HUD = PC ? PC->GetHUD<ASFHUD>() : nullptr)
		{
			ASFPlayerState* PS = PC->GetPlayerState<ASFPlayerState>();
			AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState<AGameStateBase>() : nullptr;
			TArray<AActor*> ChildActors;
			HUD->InitSharedOverlay(PC, PS, GS);
		}
	}
}