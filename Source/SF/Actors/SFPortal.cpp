#include "SFPortal.h"

#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/SFGameState.h"
#include "GameModes/SFPortalManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/SFPlayerState.h"

ASFPortal::ASFPortal()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true; // 모든 클라이언트에게 항상 관련성 있음

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(Root);
	InteractionBox->SetBoxExtent(FVector(150.f, 150.f, 100.f));
	InteractionBox->SetCollisionProfileName(TEXT("Interactable"));
	
	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(Root);
	PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PortalEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalEffect"));
	PortalEffect->SetupAttachment(Root);
	PortalEffect->SetActive(false);
	PortalEffect->bAutoActivate = false;

	bIsEnabled = false;
}

void ASFPortal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASFPortal, bIsEnabled);
}

void ASFPortal::BeginPlay()
{
	Super::BeginPlay();
	
	// PortalManager에 등록
	FindAndRegisterWithManager();
}

void ASFPortal::FindAndRegisterWithManager()
{
	if (ASFGameState* SFGameState = GetWorld()->GetGameState<ASFGameState>())
	{
		if (USFPortalManagerComponent* PortalManager = SFGameState->GetPortalManager())
		{
			CachedPortalManager = PortalManager;
			PortalManager->RegisterPortal(this);
		}
	}
}

FSFInteractionInfo ASFPortal::GetPreInteractionInfo(const FSFInteractionQuery& InteractionQuery) const
{
	if (AController* Controller = InteractionQuery.RequestingController.Get())
	{
		if (IsPlayerReady(Controller->PlayerState))
		{
			return CancelReadyInteractionInfo;
		}
	}
	return ReadyInteractionInfo;
}

bool ASFPortal::CanInteraction(const FSFInteractionQuery& InteractionQuery) const
{
	if (!ISFInteractable::CanInteraction(InteractionQuery))
	{
		return false;
	}
	
	if (!bIsEnabled)
	{
		return false;
	}

	// Travel 카운트다운 중이면 상호작용 불가 (Ready 취소 방지)
	if (CachedPortalManager && CachedPortalManager->IsTravelCountdownActive())
	{
		return false;
	}

	return true;
}

void ASFPortal::SetPortalEnabled(bool bEnabled)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsEnabled = bEnabled;

	// Listen 서버 비주얼 업데이트
	UpdatePortalEffects();

	if (bEnabled)
	{
		MulticastPlayActivateSound();
	}
}

void ASFPortal::TogglePlayerReady(APlayerState* PlayerState)
{
	if (!HasAuthority() || !PlayerState)
	{
		return;
	}

	if (CachedPortalManager)
	{
		CachedPortalManager->TogglePlayerReady(PlayerState);
	}
}

bool ASFPortal::IsPlayerReady(APlayerState* PlayerState) const
{
	if (const ASFPlayerState* SFPS = Cast<ASFPlayerState>(PlayerState))
	{
		return SFPS->GetIsReadyForTravel();
	}
	return false;
}

void ASFPortal::UpdatePortalEffects()
{
	if (PortalEffect)
	{
		PortalEffect->SetActive(bIsEnabled);
	}

	// 루프 사운드
	if (bIsEnabled)
	{
		if (PortalLoopSound && !LoopAudioComponent)
		{
			LoopAudioComponent = UGameplayStatics::SpawnSoundAttached(
				PortalLoopSound,
				PortalMesh,
				NAME_None,
				FVector::ZeroVector,
				EAttachLocation::SnapToTarget,
				true,  // bStopWhenAttachedToDestroyed
				1.0f,  // VolumeMultiplier
				1.0f,  // PitchMultiplier
				0.0f,  // StartTime
				nullptr,
				nullptr,
				false  // bAutoDestroy - 수동 제어
			);
		}
	}
	else
	{
		if (LoopAudioComponent)
		{
			LoopAudioComponent->Stop();
			LoopAudioComponent->DestroyComponent();
			LoopAudioComponent = nullptr;
		}
	}
}

void ASFPortal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// PortalManager에서 등록 해제
	if (CachedPortalManager)
	{
		CachedPortalManager->UnregisterPortal(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ASFPortal::OnRep_bIsEnabled()
{
	UpdatePortalEffects();
}

void ASFPortal::MulticastPlayActivateSound_Implementation()
{
	if (PortalActivateSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PortalActivateSound, GetActorLocation());
	}
}



