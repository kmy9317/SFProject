#include "SFMultiGroundActor.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Libraries/SFCombatLibrary.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

ASFMultiGroundActor::ASFMultiGroundActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 부모의 구체 컴포넌트 비활성화
	if (AreaCollision)
	{
		AreaCollision->SetCollisionProfileName(TEXT("NoCollision"));
		AreaCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 원기둥(Capsule) 생성
	LightningCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("LightningCollision"));
	LightningCollision->SetupAttachment(RootComponent);
	
	// [중요] 초기 크기는 블루프린트 설정을 따르기 위해 여기서 강제하지 않음
	// 기본적으로 OverlapAllDynamic 등으로 설정 추천
	LightningCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic")); 
}

void ASFMultiGroundActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, LightningScale);
}

void ASFMultiGroundActor::InitLightning(UAbilitySystemComponent* InSourceASC, AActor* InSourceActor, float InBaseDamage, float InBoltRadius, float InBoltHeight, float InScale )
{
	// 기본 데이터 저장
	SourceASC = InSourceASC;
	SourceActor = InSourceActor;
	BaseDamage = InBaseDamage;
	AttackRadius = InBoltRadius;
	LightningScale = InScale;

	SetActorScale3D(FVector(LightningScale));
	if (HasAuthority())
	{
		// 실제 데미지 판정 실행 (아래 오버라이드된 함수가 호출됨)
		ApplyDamageToTargets(BaseDamage, 0.0f); 
	}

	// 자동 파괴
	SetLifeSpan(1.5f); 
}

void ASFMultiGroundActor::BeginPlay()
{
	Super::BeginPlay();
}

void ASFMultiGroundActor::ApplyDamageToTargets(float DamageAmount, float EffectRadius)
{
	if (!SourceASC.IsValid())
	{
		return;
	}

	if (!LightningCollision)
	{
		return;
	}

	// 캡슐의 실제 월드 스케일 적용된 크기 사용
	float CapsuleRadius = 0.f;
	float CapsuleHalfHeight = 0.f;
	LightningCollision->GetScaledCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

	FSFAreaDamageParams Params;
	Params.SourceASC = SourceASC.Get();
	Params.SourceActor = SourceActor.Get();
	Params.EffectCauser = this;
	Params.Origin = GetActorLocation();
	Params.OverlapShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
	Params.DamageAmount = DamageAmount;
	Params.DamageSetByCallerTag = SetByCallerDamageTag;
	Params.DamageGEClass = DamageGameplayEffectClass;
	Params.DebuffGEClass = DebuffGameplayEffectClass;
	Params.IgnoreActors = { this, SourceActor.Get() };

	USFCombatLibrary::ApplyAreaDamage(Params);
}

void ASFMultiGroundActor::UpdateAOESize()
{
	
}

void ASFMultiGroundActor::OnRep_LightningScale()
{
	SetActorScale3D(FVector(LightningScale));
}
