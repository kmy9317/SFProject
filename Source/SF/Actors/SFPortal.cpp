#include "SFPortal.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "SFLogChannels.h"
#include "GameFramework/PlayerState.h"
#include "System/SFGameInstance.h"


ASFPortal::ASFPortal()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true; // 모든 클라이언트에게 항상 관련성 있음

	// Root Component
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(150.0f, 150.0f, 200.0f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetGenerateOverlapEvents(true);

	// Portal Mesh
	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(RootComponent);
	PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Portal Effect
	PortalEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("PortalEffect"));
	PortalEffect->SetupAttachment(RootComponent);
	PortalEffect->bAutoActivate = false;

	// Default Values
	bIsEnabled = false;
	TravelDelayTime = 3.0f;
	bIsTraveling = false;
}

void ASFPortal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASFPortal, bIsEnabled);
	DOREPLIFETIME(ASFPortal, PlayersInPortal);
}

void ASFPortal::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 Overlap 이벤트 바인딩
	if (HasAuthority())
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ASFPortal::OnPortalBeginOverlap);
		TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ASFPortal::OnPortalEndOverlap);
	}

	// 초기 상태 설정
	if (PortalEffect)
	{
		PortalEffect->SetActive(bIsEnabled);
	}
}

void ASFPortal::SetPortalEnabled(bool bEnabled)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bIsEnabled == bEnabled)
	{
		return;
	}

	bIsEnabled = bEnabled;

	if (PortalEffect)
	{
		PortalEffect->SetActive(bIsEnabled);
	}

	// 활성화 시 브로드캐스트
	if (bIsEnabled)
	{
		OnPortalActivated.Broadcast();
	}

	// 비활성화 시 타이머 취소 및 플레이어 목록 초기화
	if (!bIsEnabled)
	{
		GetWorld()->GetTimerManager().ClearTimer(TravelTimerHandle);
		PlayersInPortal.Empty();
		bIsTraveling = false;
	}

	BroadcastPlayerCountChanged();

	UE_LOG(LogSF, Warning, TEXT("[Portal] Portal %s at %s"), 
		bIsEnabled ? TEXT("Enabled") : TEXT("Disabled"), 
		*GetActorLocation().ToString());
}

void ASFPortal::OnPortalBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bIsEnabled || bIsTraveling)
	{
		return;
	}

	// Pawn이 진입했는지 확인
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			// 이미 목록에 있는지 확인
			if (!PlayersInPortal.Contains(PC))
			{
				PlayersInPortal.AddUnique(PC);
				
				UE_LOG(LogSF, Log, TEXT("[Portal] Player %s entered portal (%d/%d)"), 
					*PC->GetPlayerState<APlayerState>()->GetPlayerName(),
					PlayersInPortal.Num(), 
					GetRequiredPlayerCount());

				BroadcastPlayerCountChanged();

				// 모든 플레이어가 입장했는지 확인
				if (IsReadyToTravel())
				{
					UE_LOG(LogSF, Log, TEXT("[Portal] All players ready! Starting travel in %.1f seconds"), 
						TravelDelayTime);

					// 딜레이 후 전환
					GetWorld()->GetTimerManager().SetTimer(
						TravelTimerHandle,
						this,
						&ASFPortal::TravelToNextStage,
						TravelDelayTime,
						false
					);
				}
			}
		}
	}
}

void ASFPortal::OnPortalEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority() || bIsTraveling)
	{
		return;
	}

	// Pawn이 이탈했는지 확인
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			if (PlayersInPortal.Remove(PC) > 0)
			{
				UE_LOG(LogSF, Log, TEXT("[Portal] Player %s left portal (%d/%d)"), 
					*PC->GetPlayerState<APlayerState>()->GetPlayerName(),
					PlayersInPortal.Num(), 
					GetRequiredPlayerCount());

				// 타이머 취소
				GetWorld()->GetTimerManager().ClearTimer(TravelTimerHandle);

				BroadcastPlayerCountChanged();
			}
		}
	}
}

bool ASFPortal::IsReadyToTravel() const
{
	return bIsEnabled && PlayersInPortal.Num() >= GetRequiredPlayerCount() && !bIsTraveling;
}

int32 ASFPortal::GetRequiredPlayerCount() const
{
	if (const AGameStateBase* GameState = GetWorld()->GetGameState())
	{
		return GameState->PlayerArray.Num();
	}
	return 1;
}

void ASFPortal::TravelToNextStage()
{
	if (!HasAuthority() || bIsTraveling)
	{
		return;
	}

	// 여전히 모든 플레이어가 있는지 재확인
	if (!IsReadyToTravel())
	{
		UE_LOG(LogSF, Warning, TEXT("[Portal] Travel cancelled - not all players in portal"));
		return;
	}

	bIsTraveling = true;

	if (NextStageLevel.IsNull())
	{
		UE_LOG(LogSF, Error, TEXT("[Portal] NextStageLevel is not set!"));
		return;
	}

	UE_LOG(LogSF, Log, TEXT("[Portal] Traveling to next stage: %s"), 
		*NextStageLevel.ToString());

	// GameInstance를 통해 맵 전환
	if (USFGameInstance* GameInstance = GetWorld()->GetGameInstance<USFGameInstance>())
	{
		GameInstance->LoadLevelAndListen(NextStageLevel);
	}
}

void ASFPortal::OnRep_bIsEnabled()
{
	if (PortalEffect)
	{
		PortalEffect->SetActive(bIsEnabled);
	}

	BroadcastPlayerCountChanged();
}

void ASFPortal::OnRep_PlayersInPortal()
{
	BroadcastPlayerCountChanged();
}

void ASFPortal::BroadcastPlayerCountChanged()
{
	OnPlayerCountChanged.Broadcast(PlayersInPortal.Num(), GetRequiredPlayerCount());
	
	// 블루프린트 이벤트 호출
	OnPortalStateChanged(bIsEnabled, PlayersInPortal.Num(), GetRequiredPlayerCount());
}


