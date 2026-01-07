#include "SFMultiGroundActor.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/OverlapResult.h" 
#include "AbilitySystemComponent.h"
#include "Character/SFCharacterBase.h"
#include "System/SFAssetManager.h"
#include "System/Data/SFGameData.h"

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
	
	// [변경] AttackRadius는 더 이상 판정에 직접 쓰이지 않지만, 참조용으로 저장할 수 있음.
	// 실제 판정은 컴포넌트 크기를 따름.
	AttackRadius = InBoltRadius; 

	// [삭제됨] 강제로 콜리전 크기를 덮어쓰는 코드 제거
	// LightningCollision->SetCapsuleSize(InBoltRadius, InBoltHeight); 

	// [삭제됨] VFX 스케일을 따로 조절하는 코드 제거 (액터 전체 스케일로 제어됨)
	/*
	float WidthScale = InBoltRadius / 50.0f;
	FVector NewScale = FVector(WidthScale, WidthScale, 1.0f);
	if (AreaEffect) AreaEffect->SetWorldScale3D(NewScale);
	if (AreaEffectCascade) AreaEffectCascade->SetWorldScale3D(NewScale);
	*/

	// === 즉시 타격 로직 ===
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

// [중요] 부모 함수 오버라이드: 인자로 받은 반경 무시하고 '실제 캡슐 크기'로 판정
void ASFMultiGroundActor::ApplyDamageToTargets(float DamageAmount, float EffectRadius)
{
	if (!SourceASC.IsValid()) return;

	// 1. 현재 캡슐 컴포넌트의 실제 크기(월드 스케일 적용됨)를 가져옴
	float CapsuleRadius = 0.f;
	float CapsuleHalfHeight = 0.f;
	
	if (LightningCollision)
	{
		// BP에서 설정한 크기에 Actor Scale이 곱해진 최종 크기가 나옵니다.
		LightningCollision->GetScaledCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	}
	else
	{
		return; // 캡슐이 없으면 판정 불가
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (AActor* Src = SourceActor.Get()) QueryParams.AddIgnoredActor(Src);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	// 2. 가져온 캡슐 크기로 오버랩 검사 (MakeCapsule)
	bool bHit = GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight),
		QueryParams
	);

	if (!bHit) return;

	// 3. 데미지 적용 (부모 로직과 동일)
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

		// 아군 오사 방지
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