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
#include "Net/UnrealNetwork.h"

ASFGroundAOE::ASFGroundAOE(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	AreaCollision = CreateDefaultSubobject<USphereComponent>(TEXT("AreaCollision"));
	RootComponent = AreaCollision;
	AreaCollision->SetCollisionProfileName(TEXT("NoCollision"));
	
	AreaEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("AreaEffect"));
	AreaEffect->SetupAttachment(RootComponent);

	AreaEffectCascade = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("AreaEffectCascade"));
	AreaEffectCascade->SetupAttachment(RootComponent);
	
	SetByCallerDamageTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage.BaseDamage"), false);
}

void ASFGroundAOE::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// AttackRadius 변수를 복제 목록에 등록
	DOREPLIFETIME(ASFGroundAOE, AttackRadius);
}

// [변경] 인자가 유효할 때만 변수를 덮어씌움
void ASFGroundAOE::InitAOE(UAbilitySystemComponent* InSourceASC, AActor* InSourceActor, float InBaseDamage, float InRadius, float InDuration, float InTickInterval, float InExplosionRadius, float InExplosionDamageMultiplier, bool bOverrideExplodeOnEnd, bool bForceExplode)
{
	SourceASC = InSourceASC;
	SourceActor = InSourceActor;
	BaseDamage = InBaseDamage;
	AttackRadius = InRadius;

	UpdateAOESize();
	
	// === 폭발 설정 처리 ===
	// 1. 폭발 반경: 인자가 유효(>0)하면 덮어쓰고, 아니면 에디터 설정값 유지
	if (InExplosionRadius > 0.f)
	{
		ExplosionRadius = InExplosionRadius;
	}

	// 2. 데미지 배율: 인자가 유효(>=0)하면 덮어쓰고, 아니면 에디터 설정값 유지
	if (InExplosionDamageMultiplier >= 0.f)
	{
		ExplosionDamageMultiplier = InExplosionDamageMultiplier;
	}

	// 3. 폭발 여부: 오버라이드 플래그가 true일 때만 인자값 적용
	if (bOverrideExplodeOnEnd)
	{
		bExplodeOnEnd = bForceExplode;
	}

	// 충돌체 크기는 기본 공격(Tick) 범위로 설정
	AreaCollision->SetSphereRadius(AttackRadius);
	
	// 이펙트 스케일 (기본 범위 기준)
	float Scale = AttackRadius / 100.f; 
	FVector NewScale = FVector(Scale, Scale, 1.0f);

	if (AreaEffect) AreaEffect->SetWorldScale3D(NewScale);
	if (AreaEffectCascade) AreaEffectCascade->SetWorldScale3D(NewScale);

	if (HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(
			DurationTimerHandle,
			this,
			&ASFGroundAOE::OnDurationExpired,
			InDuration,
			false
		);

		if (InTickInterval > 0.f)
		{
			GetWorld()->GetTimerManager().SetTimer(
				TickTimerHandle,
				this,
				&ASFGroundAOE::OnDamageTick,
				InTickInterval,
				true, 
				0.f   
			);
		}
	}
}

void ASFGroundAOE::OnRep_AttackRadius()
{
	UpdateAOESize();
}

// [추가] 실제 크기를 변경하는 로직 분리
void ASFGroundAOE::UpdateAOESize()
{
	// 충돌체 크기 설정
	if (AreaCollision)
	{
		AreaCollision->SetSphereRadius(AttackRadius);
	}
    
	// 이펙트 스케일 (기본 범위 기준)
	float Scale = AttackRadius / 100.f; 
	FVector NewScale = FVector(Scale, Scale, 1.0f);

	if (AreaEffect) AreaEffect->SetWorldScale3D(NewScale);
	if (AreaEffectCascade) AreaEffectCascade->SetWorldScale3D(NewScale);
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
	if (bExplodeOnEnd)
	{
		ExecuteExplosion();
	}
	else
	{
		ExecuteRemovalGameplayCue();
	}
	
	Destroy();
}

void ASFGroundAOE::ExecuteExplosion()
{
	// 1. 서버: 데미지 적용
	if (HasAuthority())
	{
		float FinalExplosionDamage = BaseDamage * ExplosionDamageMultiplier;
		
		// 에디터에서 설정된(혹은 InitAOE로 덮어씌워진) ExplosionRadius 사용
		ApplyDamageToTargets(FinalExplosionDamage, ExplosionRadius);
	}

	// 2. GameplayCue 실행
	if (SourceASC.IsValid() && ExplosionGameplayCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = GetActorLocation();
		CueParams.Instigator = SourceActor.Get();
		CueParams.EffectCauser = this;
		
		// GC에 폭발 반경 전달 (VFX 스케일링 등에 사용 가능)
		CueParams.RawMagnitude = ExplosionRadius; 

		SourceASC->ExecuteGameplayCue(ExplosionGameplayCueTag, CueParams);
	}
}

void ASFGroundAOE::OnDamageTick()
{
	if (HasAuthority())
	{
		// 틱 데미지는 기본 반경(AttackRadius) 사용
		ApplyDamageToTargets(BaseDamage, AttackRadius);
	}
	
	if (TickSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, TickSound, GetActorLocation());
	}
}

void ASFGroundAOE::ApplyDamageToTargets(float DamageAmount, float EffectRadius)
{
	if (!SourceASC.IsValid()) return;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (AActor* Src = SourceActor.Get()) QueryParams.AddIgnoredActor(Src);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	// 인자로 받은 EffectRadius 사용
	bool bHit = GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(EffectRadius),
		QueryParams
	);

	if (!bHit) return;

	TSubclassOf<UGameplayEffect> DamageGE = DamageGameplayEffectClass;
	if (!DamageGE)
	{
		DamageGE = USFAssetManager::GetSubclassByPath(USFGameData::Get().DamageGameplayEffect_SetByCaller);
	}

	if (!DamageGE) return;

	TSet<AActor*> ProcessedActors;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!TargetActor || ProcessedActors.Contains(TargetActor)) continue;

		ProcessedActors.Add(TargetActor);

		// 아군 피격 방지 로직
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

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (TargetASC)
		{
			FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
			ContextHandle.AddInstigator(SourceActor.Get(), this);
			
			FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageGE, 1.0f, ContextHandle);
			if (SpecHandle.IsValid())
			{
				if (SetByCallerDamageTag.IsValid())
				{
					SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerDamageTag, DamageAmount);
				}
				SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			}

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

void ASFGroundAOE::ExecuteRemovalGameplayCue()
{
	if (HasAuthority() && RemoveGameplayCueTag.IsValid() && SourceASC.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = GetActorLocation();
		CueParams.Instigator = SourceActor.Get();
		CueParams.EffectCauser = this;
		
		SourceASC->ExecuteGameplayCue(RemoveGameplayCueTag, CueParams);
	}
}