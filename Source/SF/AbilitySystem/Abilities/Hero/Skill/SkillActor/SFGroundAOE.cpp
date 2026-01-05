#include "SFGroundAOE.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "System/SFAssetManager.h"
#include "System/Data/SFGameData.h"
#include "Character/SFCharacterBase.h"
#include "Engine/OverlapResult.h"

ASFGroundAOE::ASFGroundAOE(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// 루트 컴포넌트: 충돌 범위
	AreaCollision = CreateDefaultSubobject<USphereComponent>(TEXT("AreaCollision"));
	RootComponent = AreaCollision;
	AreaCollision->SetCollisionProfileName(TEXT("NoCollision")); // Overlap 검사는 쿼리로 직접 수행
	
	// 시각 효과
	AreaEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("AreaEffect"));
	AreaEffect->SetupAttachment(RootComponent);
	
	// 기본 SetByCaller 태그 설정 (SFAttackProjectile과 동일하게 맞춤)
	SetByCallerDamageTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage.BaseDamage"), false);
}

void ASFGroundAOE::InitAOE(UAbilitySystemComponent* InSourceASC, AActor* InSourceActor, float InBaseDamage, float InRadius, float InDuration, float InTickInterval)
{
	SourceASC = InSourceASC;
	SourceActor = InSourceActor;
	BaseDamage = InBaseDamage;
	AttackRadius = InRadius;

	// 범위 적용
	AreaCollision->SetSphereRadius(AttackRadius);
	
	// 이펙트 스케일 조정 (Niagara 파라미터로 넘길 수도 있지만, 여기선 액터/컴포넌트 스케일로 단순화)
	// 기본 반경이 100이라고 가정할 때의 비율 계산 예시
	float Scale = AttackRadius / 100.f; 
	AreaEffect->SetWorldScale3D(FVector(Scale, Scale, 1.0f)); // 높이는 유지하거나 조정

	// 타이머 시작
	if (HasAuthority())
	{
		// 지속 시간 타이머
		GetWorld()->GetTimerManager().SetTimer(
			DurationTimerHandle,
			this,
			&ASFGroundAOE::OnDurationExpired,
			InDuration,
			false
		);

		// 틱 데미지 타이머
		if (InTickInterval > 0.f)
		{
			GetWorld()->GetTimerManager().SetTimer(
				TickTimerHandle,
				this,
				&ASFGroundAOE::OnDamageTick,
				InTickInterval,
				true, // Loop
				0.f   // First delay (즉시 발동)
			);
		}
	}
}

void ASFGroundAOE::BeginPlay()
{
	Super::BeginPlay();

	if (SpawnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SpawnSound, GetActorLocation());
	}
}

void ASFGroundAOE::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(DurationTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(TickTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void ASFGroundAOE::OnDurationExpired()
{
	Destroy();
}

void ASFGroundAOE::OnDamageTick()
{
	if (HasAuthority())
	{
		ApplyDamageEffect_Server();
	}
	
	// 클라이언트: 틱 사운드 재생
	if (TickSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, TickSound, GetActorLocation());
	}
}

void ASFGroundAOE::ApplyDamageEffect_Server()
{
	if (!SourceASC.IsValid()) return;

	// 범위 내 대상 탐지
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (AActor* Src = SourceActor.Get()) QueryParams.AddIgnoredActor(Src);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	bool bHit = GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(AttackRadius),
		QueryParams
	);

	if (!bHit) return;

	// 데미지 GE 클래스 결정 (없으면 기본값)
	TSubclassOf<UGameplayEffect> DamageGE = DamageGameplayEffectClass;
	if (!DamageGE)
	{
		DamageGE = USFAssetManager::GetSubclassByPath(USFGameData::Get().DamageGameplayEffect_SetByCaller);
	}

	if (!DamageGE) return;

	// 중복 피격 방지용 (한 틱 내에서)
	TSet<AActor*> ProcessedActors;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!TargetActor || ProcessedActors.Contains(TargetActor)) continue;

		ProcessedActors.Add(TargetActor);

		// 아군 판정 (Projectile 코드 참고)
		if (ASFCharacterBase* SourceChar = Cast<ASFCharacterBase>(SourceActor.Get()))
		{
			if (ASFCharacterBase* TargetChar = Cast<ASFCharacterBase>(TargetActor))
			{
				if (SourceChar->GetTeamAttitudeTowards(*TargetChar) == ETeamAttitude::Friendly)
				{
					continue; 
				}
			}
		}

		// GE 적용
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (TargetASC)
		{
			// Context 생성
			FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
			ContextHandle.AddInstigator(SourceActor.Get(), this);
			
			// Spec 생성 및 데미지 주입
			FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageGE, 1.0f, ContextHandle);
			if (SpecHandle.IsValid())
			{
				if (SetByCallerDamageTag.IsValid())
				{
					SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerDamageTag, BaseDamage);
				}
				SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			}

			// 추가 디버프 적용
			if (DebuffGameplayEffectClass)
			{
				FGameplayEffectSpecHandle DebuffSpec = SourceASC->MakeOutgoingSpec(DebuffGameplayEffectClass, 1.0f, ContextHandle);
				if (DebuffSpec.IsValid())
				{
					SourceASC->ApplyGameplayEffectSpecToTarget(*DebuffSpec.Data.Get(), TargetASC);
				}
			}
		}
	}
}