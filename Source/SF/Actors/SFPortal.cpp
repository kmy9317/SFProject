#include "SFPortal.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "SFLogChannels.h"
#include "GameFramework/PlayerState.h"
#include "Player/SFPlayerState.h"
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

	// Listen 서버 로직
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
}

int32 ASFPortal::GetRequiredPlayerCount() const
{
	if (const AGameStateBase* GameState = GetWorld()->GetGameState())
	{
		// 유효한 PlayerState만 카운트
		int32 ValidPlayerCount = 0;
		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (PS && !PS->IsInactive())
			{
				ValidPlayerCount++;
			}
		}
		return ValidPlayerCount;
	}
	return 1;
}

bool ASFPortal::IsReadyToTravel() const
{
	return bIsEnabled && PlayersInPortal.Num() >= GetRequiredPlayerCount() && !bIsTraveling && GetRequiredPlayerCount() > 0;
}

void ASFPortal::OnPortalBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bIsEnabled || bIsTraveling)
	{
		return;
	}

	// Pawn이 진입했는지 확인
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	// PlayerState 가져오기
	APlayerState* PS = Pawn->GetPlayerState();
	if (!PS || PS->IsInactive())
	{
		return;
	}

	// 이미 목록에 있는지 확인
	if (IsPlayerInPortal(PS))
	{
		return;
	}

	// PlayerSelectionInfo 가져오기
	FSFPlayerSelectionInfo PlayerInfo = GetPlayerSelectionInfo(PS);
	
	// 플레이어 추가
	PlayersInPortal.Add(PlayerInfo);

	// 델리게이트 브로드캐스트
	OnPlayerEntered.Broadcast(PlayerInfo);
	BroadcastPlayerCountChanged();

	// 모든 플레이어가 입장했는지 확인
	if (IsReadyToTravel())
	{
		UE_LOG(LogSF, Warning, TEXT("[Portal] All players ready! Starting travel in %.1f seconds"), TravelDelayTime);

		// 딜레이 후 전환
		GetWorld()->GetTimerManager().SetTimer(TravelTimerHandle,this, &ASFPortal::TravelToNextStage,TravelDelayTime,false);
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
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	// PlayerState 가져오기
	APlayerState* PS = Pawn->GetPlayerState();
	if (!PS)
	{
		return;
	}

	// 플레이어 찾기
	int32 FoundIndex = PlayersInPortal.IndexOfByPredicate([PS](const FSFPlayerSelectionInfo& Info)
	{
		return Info.IsForPlayer(PS);
	});

	if (FoundIndex != INDEX_NONE)
	{
		FSFPlayerSelectionInfo RemovedPlayer = PlayersInPortal[FoundIndex];
		PlayersInPortal.RemoveAt(FoundIndex);

		// 델리게이트 브로드캐스트
		OnPlayerLeft.Broadcast(RemovedPlayer);

		// 타이머 취소
		GetWorld()->GetTimerManager().ClearTimer(TravelTimerHandle);

		BroadcastPlayerCountChanged();
	}
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

	UE_LOG(LogSF, Log, TEXT("[Portal] Traveling to next stage: %s"), *NextStageLevel.ToString());

	// Seamless Travel 실행
	if (USFGameInstance* SFGameInstance = Cast<USFGameInstance>(GetWorld()->GetGameInstance()))
	{
		SFGameInstance->LoadLevelAndListen(NextStageLevel);
	}
}

bool ASFPortal::IsPlayerInPortal(const APlayerState* PlayerState) const
{
	if (!PlayerState)
	{
		return false;
	}

	return PlayersInPortal.ContainsByPredicate([PlayerState](const FSFPlayerSelectionInfo& Info)
	{
		return Info.IsForPlayer(PlayerState);
	});
}

FSFPlayerSelectionInfo ASFPortal::GetPlayerSelectionInfo(const APlayerState* PlayerState) const
{
	// ASFPlayerState에서 PlayerSelection 가져오기
	if (const ASFPlayerState* SFPS = Cast<ASFPlayerState>(PlayerState))
	{
		return SFPS->GetPlayerSelection();
	}

	// ASFPlayerState가 아니면 기본 정보만 생성
	FSFPlayerSelectionInfo DefaultInfo;
	if (PlayerState)
	{
		DefaultInfo = FSFPlayerSelectionInfo(0, PlayerState);
	}
	return DefaultInfo;
}

void ASFPortal::BroadcastPlayerCountChanged()
{
	const int32 CurrentCount = PlayersInPortal.Num();
	const int32 RequiredCount = GetRequiredPlayerCount();
	
	OnPlayerCountChanged.Broadcast(CurrentCount, RequiredCount);
	
	// 블루프린트 이벤트 호출
	OnPortalStateChanged(bIsEnabled, CurrentCount, RequiredCount);
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


