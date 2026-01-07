#include "SFGC_FXSpawnFloor.h"
#include "NiagaraComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

ASFGC_FXSpawnFloor::ASFGC_FXSpawnFloor()
{
	// 틱(Tick)은 기본적으로 필요 없으므로 꺼둡니다 (최적화)
	PrimaryActorTick.bCanEverTick = false;
	
	// GameplayCue가 끝나면(OnRemove) 자동으로 Actor를 Destroy 할지 여부
	bAutoDestroyOnRemove = true;

	// 루트 컴포넌트 생성 (위치 기준점)
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// 1. 나이아가라 컴포넌트 초기화
	NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComp"));
	NiagaraComp->SetupAttachment(RootComponent);
	NiagaraComp->bAutoActivate = false; // OnActive에서 수동 제어

	// 2. 캐스케이드 컴포넌트 초기화
	CascadeComp = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("CascadeComp"));
	CascadeComp->SetupAttachment(RootComponent);
	CascadeComp->bAutoActivate = false;

	// 3. 사운드 컴포넌트 초기화
	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComp"));
	AudioComp->SetupAttachment(RootComponent);
	AudioComp->bAutoActivate = false;
}

bool ASFGC_FXSpawnFloor::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	// 1. Target 유효성 검사
	if (!MyTarget)
	{
		return false; // Target이 없으면 아무것도 하지 않고 종료
	}

	FVector SpawnLocation = MyTarget->GetActorLocation();
	FRotator SpawnRotation = MyTarget->GetActorRotation();

	// 2. 캐릭터 바닥 위치 보정
	ACharacter* Character = Cast<ACharacter>(MyTarget);
	if (Character && Character->GetCapsuleComponent())
	{
		float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		SpawnLocation.Z -= CapsuleHalfHeight;
	}

	SpawnLocation += FloorOffset;
	SpawnRotation += AdditionalRotation;

	// 3. RootComponent 유효성 검사 (SetActorLocation 등은 RootComponent가 있어야 함)
	if (RootComponent)
	{
		SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
		SetActorScale3D(EffectScale);

		if (bFollowCharacter)
		{
			FAttachmentTransformRules AttachRules(EAttachmentRule::KeepWorld, true);
			AttachToActor(MyTarget, AttachRules);
		}
	}

	// 4. 컴포넌트 유효성 검사 후 실행 (여기가 크래시의 주 원인!)
	// 반드시 '&&' 연산자를 사용하여 포인터가 null이 아닌지 먼저 확인해야 합니다.

	// Niagara 실행
	if (NiagaraComp && NiagaraComp->GetAsset()) 
	{
		// 덮어쓰기 방지를 위해 Reset 후 실행하거나 바로 Activate
		NiagaraComp->Activate(true);
	}

	// Cascade 실행
	if (CascadeComp && CascadeComp->Template)
	{
		CascadeComp->Activate(true);
	}

	// Audio 실행
	if (AudioComp && AudioComp->Sound)
	{
		AudioComp->Play();
	}

	return Super::OnActive_Implementation(MyTarget, Parameters);
}

bool ASFGC_FXSpawnFloor::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (NiagaraComp) 
	{
		NiagaraComp->Deactivate();
	}

	if (CascadeComp) 
	{
		CascadeComp->Deactivate();
	}

	if (AudioComp) 
	{
		AudioComp->Stop();
	}
	
	return Super::OnRemove_Implementation(MyTarget, Parameters);
}