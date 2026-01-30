#include "SFPlayerSlot.h"
#include "Components/ArrowComponent.h"
#include "Character/Hero/SFHeroDefinition.h"
#include "Actors/SFHeroDisplay.h"
#include "SFLogChannels.h"
#include "GameModes/Lobby/SFLobbyGameMode.h"
#include "Player/Lobby/SFLobbyPlayerState.h"

ASFPlayerSlot::ASFPlayerSlot()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root
	USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootComp);

	// Arrow
	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(RootComp);

	// 초기화
	CachedPC = nullptr;
	HeroDisplay = nullptr;
}

void ASFPlayerSlot::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// HeroDisplay 미리 스폰
		SpawnHeroDisplay();
		
		// PlayerSlot 등록 요청
		if (ASFLobbyGameMode* LobbyGM = GetWorld()->GetAuthGameMode<ASFLobbyGameMode>())
		{
			LobbyGM->RegisterPlayerSlot(this);
		}
	}
}

void ASFPlayerSlot::SpawnHeroDisplay()
{
	if (HeroDisplay)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Arrow 위치에 HeroDisplay 스폰
	FTransform SpawnTransform = Arrow->GetComponentTransform();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	if (HeroDisplayClass)
	{
		HeroDisplay = World->SpawnActor<ASFHeroDisplay>(
			HeroDisplayClass, 
			SpawnTransform, 
			SpawnParams
		);
	}
	else
	{
		UE_LOG(LogSF, Error, TEXT("[PlayerSlot] HeroDisplayClass is null"));
	}
	
	if (!HeroDisplay)
	{
		UE_LOG(LogSF, Error, TEXT("[PlayerSlot] Failed to spawn HeroDisplay"));
		return;
	}

	// 초기에는 숨김
	HeroDisplay->SetActorHiddenInGame(true);
}

void ASFPlayerSlot::AddPlayer(APlayerController* InPC)
{
	// PC 저장
	CachedPC = InPC;
}

void ASFPlayerSlot::RemovePlayer(APlayerController* PC)
{
	CachedPC = nullptr;

	if (!HeroDisplay)
	{
		return;
	}

	// HeroDisplay 숨김
	HeroDisplay->SetActorHiddenInGame(true);

	UE_LOG(LogSF, Log, TEXT("[PlayerSlot] RemovePlayer"));
}

void ASFPlayerSlot::UpdatePlayerDisplay(USFHeroDefinition* HeroDef, const FSFPlayerInfo& PlayerInfo)
{
	if (!HeroDisplay)
	{
		return;
	}
	
	// 1. Hero 변경 
	UpdateHeroDisplay(HeroDef);
	
	// 2. PlayerInfo Widget 업데이트(UpdateHeroInfo)
	HeroDisplay->UpdatePlayerInfo(PlayerInfo);
}

void ASFPlayerSlot::UpdateHeroDisplay(USFHeroDefinition* HeroDefinition)
{
	if (HeroDisplay)
	{
		HeroDisplay->SetActorHiddenInGame(false);
	}
	
	if (HeroDefinition && HeroDefinition != HeroDisplay->GetCurrentHeroDefinition())
	{
		HeroDisplay->UpdateHeroDefination(HeroDefinition);
		HeroDisplay->SetActorTransform(Arrow->GetComponentTransform());
	}
}


