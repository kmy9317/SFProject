#include "SFMultiGroundActor.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Libraries/SFCombatLibrary.h"
#include "AbilitySystemComponent.h"

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

void ASFMultiGroundActor::InitLightning(UAbilitySystemComponent* InSourceASC, AActor* InSourceActor, float InBaseDamage, float InBoltRadius, float InBoltHeight)
{
	// 기본 데이터 저장
	SourceASC = InSourceASC;
	SourceActor = InSourceActor;
	BaseDamage = InBaseDamage;
	
	//  AttackRadius는 더 이상 판정에 직접 쓰이지 않지만, 참조용으로 저장할 수 있음
	// 실제 판정은 컴포넌트 크기를 따름.
	AttackRadius = InBoltRadius; 

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

void ASFMultiGroundActor::OnDamageTick_Implementation()
{
	// 부모 틱 로직 무시
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