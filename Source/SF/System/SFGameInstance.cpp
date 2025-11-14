#include "SFGameInstance.h"

#include "SFInitGameplayTags.h"
#include "Components/GameFrameworkComponentManager.h"

void USFGameInstance::Init()
{
	Super::Init();

	// InitState를 GFCM에 등록
	UGameFrameworkComponentManager* ComponentManager = GetSubsystem<UGameFrameworkComponentManager>(this);

	if (ensure(ComponentManager))
	{
		ComponentManager->RegisterInitState(SFGameplayTags::InitState_Spawned, false, FGameplayTag());
		ComponentManager->RegisterInitState(SFGameplayTags::InitState_DataAvailable, false, SFGameplayTags::InitState_Spawned);
		ComponentManager->RegisterInitState(SFGameplayTags::InitState_DataInitialized, false, SFGameplayTags::InitState_DataAvailable);
		ComponentManager->RegisterInitState(SFGameplayTags::InitState_GameplayReady, false, SFGameplayTags::InitState_DataInitialized);
	}
}

void USFGameInstance::StartMatch()
{
	if (GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer || GetWorld()->GetNetMode() == ENetMode::NM_ListenServer)
	{
		LoadLevelAndListen(GameLevel);
	}
}

void USFGameInstance::LoadLevelAndListen(TSoftObjectPtr<UWorld> Level)
{
	const FName LevelURL = FName(*FPackageName::ObjectPathToPackageName(Level.ToString()));

	if (LevelURL != "")
	{
		//GetWorld()->ServerTravel(LevelURL.ToString() + "?listen");
		GetWorld()->ServerTravel(LevelURL.ToString(), true);
	}
}