#include "SFAttackProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GenericTeamAgentInterface.h"
#include "System/SFAssetManager.h"
#include "System/Data/SFGameData.h"
#include "AbilitySystem/GameplayEffect/SFGameplayEffectContext.h"
#include "Character/SFCharacterBase.h"
#include "Engine/OverlapResult.h"

ASFAttackProjectile::ASFAttackProjectile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	RootComponent = Collision;
	Collision->InitSphereRadius(12.f);
	Collision->SetCollisionObjectType(ECC_GameTraceChannel1);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	Collision->OnComponentHit.AddDynamic(this, &ThisClass::OnProjectileHit);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnProjectileOverlap);
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TrailNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailNiagara"));
	TrailNiagara->SetupAttachment(Collision);
	TrailNiagara->SetAutoActivate(true);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = Collision;
	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed = MaxSpeed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	// 기본 SetByCaller 태그 (프로젝트에 존재하는 태그로 BP에서 바꿔도 됨)
	SetByCallerDamageTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage.BaseDamage"), /*ErrorIfNotFound*/ false);
}

void ASFAttackProjectile::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(LifeSeconds);

	if (SpawnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SpawnSound, GetActorLocation());
	}
}

void ASFAttackProjectile::InitProjectile(UAbilitySystemComponent* InSourceASC, float InDamage, AActor* InSourceActor)
{
	SourceASC = InSourceASC;
	Damage = InDamage;
	SourceActor = InSourceActor;

	// 자기 자신(발사자) 충돌 무시
	if (AActor* Src = SourceActor.Get())
	{
		Collision->IgnoreActorWhenMoving(Src, true);
	}
}

void ASFAttackProjectile::InitProjectileCharged(UAbilitySystemComponent* InSourceASC, float InDamage, AActor* InSourceActor, float InScale, bool bInExplodes)
{
	// 기존 초기화 로직 재사용
	InitProjectile(InSourceASC, InDamage, InSourceActor);

	// 스케일 적용
	SetActorScale3D(FVector(InScale));

	// 폭발 플래그 설정
	bIsExplosive = bInExplodes;
}

void ASFAttackProjectile::Launch(const FVector& Direction)
{
	if (ProjectileMovement)
	{
		const FVector Dir = Direction.GetSafeNormal();
		ProjectileMovement->Velocity = Dir * ProjectileMovement->InitialSpeed;
	}
}

void ASFAttackProjectile::OnProjectileHit(UPrimitiveComponent* HitComp,AActor* OtherActor,UPrimitiveComponent* OtherComp,FVector NormalImpulse,const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	// 소스(발사자)면 무시
	if (AActor* Src = SourceActor.Get())
	{
		if (OtherActor == Src || OtherActor->GetOwner() == Src)
		{
			return;
		}
	}

	// Pawn은 Overlap에서 처리하므로, Hit는 월드 임팩트 연출만
	if (HasAuthority())
	{
		Multicast_PlayImpactFX(Hit.ImpactPoint, Hit.ImpactNormal);

		// 폭발 로직
		if (bIsExplosive)
		{
			ProcessExplosion_Server(Hit.ImpactPoint);
		}
	}

	if (bDestroyOnHit)
	{
		Destroy();
	}
}

void ASFAttackProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent,AActor* OtherActor,UPrimitiveComponent* OtherComp,int32 OtherBodyIndex,bool bFromSweep,const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	// 소스(발사자)면 무시
	if (AActor* Src = SourceActor.Get())
	{
		if (OtherActor == Src || OtherActor->GetOwner() == Src)
		{
			return;
		}
	}

	bool bAppliedEffect = false; // 효과(데미지 or 버프)가 적용되었는지 체크

	// 1. 캐릭터 판정 및 팀 체크
	ASFCharacterBase* TargetCharacter = Cast<ASFCharacterBase>(OtherActor);
	ASFCharacterBase* SourceCharacter = Cast<ASFCharacterBase>(SourceActor.Get());
	
	bool bIsFriendly = false;

	if (TargetCharacter && SourceCharacter)
	{
		const ETeamAttitude::Type Attitude = SourceCharacter->GetTeamAttitudeTowards(*TargetCharacter);
		bIsFriendly = (Attitude == ETeamAttitude::Friendly);
	}

	// -------------------------------------------------------
	// Case A: 아군일 경우
	// -------------------------------------------------------
	if (bIsFriendly)
	{
		// 아군 버프 옵션이 켜져 있다면 버프 적용
		if (bApplyBuffToFriendly)
		{
			if (HasAuthority())
			{
				ApplyBuff_Server(OtherActor, SweepResult);
				
				// 시각 효과 (필요하다면 아군용 FX를 따로 분리할 수도 있음)
				FVector ImpactLoc = bFromSweep ? FVector(SweepResult.ImpactPoint) : GetActorLocation();
				FVector ImpactNorm = bFromSweep ? FVector(SweepResult.ImpactNormal) : -GetVelocity().GetSafeNormal();
				Multicast_PlayImpactFX(ImpactLoc, ImpactNorm);
			}
			bAppliedEffect = true;
		}
		else
		{
			// 버프 옵션이 꺼져있다면 기존처럼 "완전 통과(무시)" 처리
			Collision->IgnoreActorWhenMoving(OtherActor, true);
			return; 
		}
	}
	// -------------------------------------------------------
	// Case B: 적(Hostile) 또는 중립일 경우
	// -------------------------------------------------------
	else 
	{
		// 서버에서만 데미지/GE/폭발 적용
		if (HasAuthority())
		{
			// 1. 직격 데미지 적용
			ApplyHitEffects_Server(OtherActor, SweepResult);
			
			// 2. 임팩트 효과
			FVector ImpactLoc = bFromSweep ? FVector(SweepResult.ImpactPoint) : GetActorLocation();
			FVector ImpactNorm = bFromSweep ? FVector(SweepResult.ImpactNormal) : -GetVelocity().GetSafeNormal();
			Multicast_PlayImpactFX(ImpactLoc, ImpactNorm);

			// 3. 폭발 로직 (아군이 아닐 때만 폭발한다고 가정)
			if (bIsExplosive)
			{
				ProcessExplosion_Server(ImpactLoc);
			}
		}
		bAppliedEffect = true;
	}

	// -------------------------------------------------------
	// 공통: 관통(Pierce) 및 파괴 처리
	// -------------------------------------------------------
	
	// 효과가 적용된 대상(적 혹은 버프받은 아군)과는 
	// 더 이상 물리적으로 부딪히지 않도록 하여 '지나가게' 만듭니다.
	// (이걸 안 하면 Overlap이 매 프레임 발생하거나 껴버릴 수 있음)
	if (bAppliedEffect)
	{
		Collision->IgnoreActorWhenMoving(OtherActor, true);
	}

	// 관통 옵션이 켜져 있으면 파괴하지 않음 (뚫고 지나감)
	if (bCanPierce)
	{
		// 관통 시에는 Destroy 호출 안 함 -> 계속 날아감
	}
	else
	{
		// 관통이 아닌데, 파괴 옵션이 켜져 있으면 파괴
		if (bDestroyOnHit)
		{
			Destroy();
		}
	}
}

TSubclassOf<UGameplayEffect> ASFAttackProjectile::ResolveDamageGE() const
{
	if (DamageGameplayEffectClass)
	{
		return DamageGameplayEffectClass;
	}

	// SFGA_Skill_Melee와 동일한 “프로젝트 공용 SetByCaller 데미지 GE” 사용
	return USFAssetManager::GetSubclassByPath(USFGameData::Get().DamageGameplayEffect_SetByCaller);
}

void ASFAttackProjectile::ApplyHitEffects_Server(AActor* TargetActor, const FHitResult& Hit)
{
	UAbilitySystemComponent* SrcASC = SourceASC.Get();
	if (!SrcASC || !TargetActor)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!TargetASC)
	{
		// 타겟이 장비/부착물이면 Owner를 캐릭터로 볼 수도 있음
		if (AActor* OwnerActor = TargetActor->GetOwner())
		{
			TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
		}
	}
	if (!TargetASC)
	{
		return;
	}

	// EffectContext 구성 (SFMeleeWeaponActor 패턴 참고)
	FGameplayEffectContextHandle ContextHandle = SrcASC->MakeEffectContext();

	// 커스텀 컨텍스트라면 Hit/SourceObject 세팅
	if (FSFGameplayEffectContext* SFContext = static_cast<FSFGameplayEffectContext*>(ContextHandle.Get()))
	{
		SFContext->AddHitResult(Hit);
		SFContext->AddSourceObject(this);
	}

	// Instigator / Causer 명시 (요구사항 3-참고)
	if (AActor* SrcActor = SourceActor.Get())
	{
		ContextHandle.AddInstigator(SrcActor, this);
	}

	const TSubclassOf<UGameplayEffect> DamageGE = ResolveDamageGE();
	if (DamageGE)
	{
		FGameplayEffectSpecHandle SpecHandle = SrcASC->MakeOutgoingSpec(DamageGE, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			if (SetByCallerDamageTag.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerDamageTag, Damage);
			}
			SrcASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}

	// (옵션) 추가 히트 GE 부여 (슬로우/화상 등)
	if (AdditionalHitGameplayEffectClass)
	{
		FGameplayEffectSpecHandle AddSpecHandle = SrcASC->MakeOutgoingSpec(AdditionalHitGameplayEffectClass, AdditionalEffectLevel, ContextHandle);
		if (AddSpecHandle.IsValid())
		{
			SrcASC->ApplyGameplayEffectSpecToTarget(*AddSpecHandle.Data.Get(), TargetASC);
		}
	}
}

void ASFAttackProjectile::Multicast_PlayImpactFX_Implementation(const FVector& Location, const FVector& Normal)
{
	if (ImpactNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactNiagara, Location, Normal.Rotation());
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Location);
	}
}

void ASFAttackProjectile::ProcessExplosion_Server(const FVector& Location)
{
	if (!SourceASC.IsValid())
	{
		return;
	}
	
    // 1. Cue 실행
    if (ExplosionCueTag.IsValid())
    {
        FGameplayCueParameters CueParams;
        CueParams.Location = Location;
        CueParams.Normal = FVector::UpVector;
        CueParams.Instigator = SourceActor.Get();
        CueParams.EffectCauser = this;
        SourceASC->ExecuteGameplayCue(ExplosionCueTag, CueParams);
    }

    // 2. GE 결정
    TSubclassOf<UGameplayEffect> EffectToApply = ExplosionGameplayEffectClass;
    if (!EffectToApply)
    {
	    EffectToApply = ResolveDamageGE();
    }
    if (!EffectToApply)
    {
	    return;
    }
    // 3. 범위 탐지
    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    if (AActor* Src = SourceActor.Get()) QueryParams.AddIgnoredActor(Src);

    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
    ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
    ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

    bool bHit = GetWorld()->OverlapMultiByObjectType(Overlaps,Location,FQuat::Identity,ObjectParams,FCollisionShape::MakeSphere(ExplosionRadius),QueryParams);
    if (bDebugExplosion)
    {
        DrawDebugSphere(GetWorld(), Location, ExplosionRadius, 12, FColor::Red, false, 2.0f);
    }

	if (!bHit)
	{
		return;
	}

	ASFCharacterBase* SourceChar = Cast<ASFCharacterBase>(SourceActor.Get());

	FGameplayEffectContextHandle SharedContext = SourceASC->MakeEffectContext();
	SharedContext.AddInstigator(SourceActor.Get(), this);

	if (FSFGameplayEffectContext* SFContext = static_cast<FSFGameplayEffectContext*>(SharedContext.Get()))
	{
		FHitResult ExplosionHit;
		ExplosionHit.ImpactPoint = Location;
		ExplosionHit.Location = Location;
		SFContext->AddHitResult(ExplosionHit);
	}

	FGameplayEffectSpecHandle SharedSpec = SourceASC->MakeOutgoingSpec(EffectToApply, 1.0f, SharedContext);
	if (!SharedSpec.IsValid())
	{
		return;
	}
	if (SetByCallerDamageTag.IsValid())
	{
		SharedSpec.Data->SetSetByCallerMagnitude(SetByCallerDamageTag, Damage);
	}

	TSet<AActor*> ProcessedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!TargetActor)
		{
			continue;
		}
		if (ProcessedActors.Contains(TargetActor))
		{
			continue;
		}
		ProcessedActors.Add(TargetActor);

		// 아군 체크
		if (SourceChar)
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
		if (!TargetASC)
		{
			continue;
		}
		SourceASC->ApplyGameplayEffectSpecToTarget(*SharedSpec.Data.Get(), TargetASC);

		if (bDebugExplosion)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Explosion] APPLIED DAMAGE to: %s / Damage: %f"), *TargetActor->GetName(), Damage);
		}
	}
}

void ASFAttackProjectile::ApplyBuff_Server(AActor* TargetActor, const FHitResult& Hit)
{
	if (!BuffGameplayEffectClass)
	{
		return;
	}
	UAbilitySystemComponent* SrcASC = SourceASC.Get();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	if (!SrcASC || !TargetASC)
	{
		return;
	}
	FGameplayEffectContextHandle ContextHandle = SrcASC->MakeEffectContext();
	if (FSFGameplayEffectContext* SFContext = static_cast<FSFGameplayEffectContext*>(ContextHandle.Get()))
	{
		SFContext->AddHitResult(Hit);
		SFContext->AddSourceObject(this);
	}
	if (AActor* SrcActor = SourceActor.Get())
	{
		ContextHandle.AddInstigator(SrcActor, this);
	}

	// 버프 GE 스펙 생성 및 적용
	FGameplayEffectSpecHandle SpecHandle = SrcASC->MakeOutgoingSpec(BuffGameplayEffectClass, 1.0f, ContextHandle);
	if (SpecHandle.IsValid())
	{
		SrcASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}