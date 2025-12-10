#include "SFPortal.h"

#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/SFGameState.h"
#include "GameModes/SFPortalManagerComponent.h"
#include "Kismet/GameplayStatics.h"

ASFPortal::ASFPortal()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true; // 모든 클라이언트에게 항상 관련성 있음

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(Root);
	TriggerBox->SetBoxExtent(FVector(150.0f, 150.0f, 200.0f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetGenerateOverlapEvents(true);

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
	
	// 서버에서만 Overlap 이벤트 바인딩
	if (HasAuthority())
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ASFPortal::OnPortalBeginOverlap);
		TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ASFPortal::OnPortalEndOverlap);
	}
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

void ASFPortal::OnPortalBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bIsEnabled)
	{
		return;
	}

	// Pawn 체크
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

	if (CachedPortalManager)
	{
		CachedPortalManager->NotifyPlayerEnteredPortal(PS);
	}
}

void ASFPortal::OnPortalEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	APlayerState* PS = Pawn->GetPlayerState();
	if (!PS)
	{
		return;
	}

	if (CachedPortalManager)
	{
		CachedPortalManager->NotifyPlayerLeftPortal(PS);
	}
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



