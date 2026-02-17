#include "SFVacuumGroundAOE.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Engine/OverlapResult.h"
#include "Libraries/SFCombatLibrary.h"

ASFVacuumGroundAOE::ASFVacuumGroundAOE(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ASFVacuumGroundAOE::BeginPlay()
{
	Super::BeginPlay();

	// 최적화를 위해 Tick 간격 조정이 필요하다면 설정
	if (PullInterval > 0.f)
	{
		SetActorTickInterval(PullInterval);
	}
}

void ASFVacuumGroundAOE::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 위치 동기화를 위해 서버(Authority)에서만 물리력을 행사
	if (!HasAuthority())
	{
		return;
	}
	
	// 직접 오버랩 쿼리 수행
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (AActor* Src = SourceActor.Get())
	{
		QueryParams.AddIgnoredActor(Src);
	}
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn); // 캐릭터(Pawn)만 감지

	// 현재 반경 가져오기
	float Radius = AreaCollision->GetScaledSphereRadius();
	bool bHit = GetWorld()->OverlapMultiByObjectType(Overlaps,GetActorLocation(),FQuat::Identity,ObjectParams,FCollisionShape::MakeSphere(Radius),QueryParams);
	if (!bHit)
	{
		return;
	}
	
	// 당기는 목표 지점 계산 (중심점 + 높이 보정)
	FVector OriginLoc = GetActorLocation();
	FVector PullTargetLoc = OriginLoc;
	PullTargetLoc.Z += PullHeightOffset; // 바닥이 아닌 공중을 향해 당김

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		
		// 유효성 검사 (아군, 사망, 비행, 무적 등 체크)
		if (!IsValidPullTarget(TargetActor))
		{
			continue;
		}
		ACharacter* TargetChar = Cast<ACharacter>(TargetActor);
		if (!TargetChar)
		{
			continue;
		}
		FVector TargetLoc = TargetActor->GetActorLocation();
		
		// 방향 벡터 계산 (공중의 점을 향해)
		FVector Direction = PullTargetLoc - TargetLoc;
		float Distance = Direction.Size();

		// 데드존 체크 (떨림 방지)
		if (Distance <= PullDeadZoneRadius)
		{
			continue;
		}
		// 방향 정규화
		Direction /= Distance;

		// 힘 적용 (LaunchCharacter)
		// PullStrength * DeltaTime * 계수(10.f) 형태로 부드러운 가속 효과
		// 높이(Offset)가 있으므로 마찰력을 무시하고 자연스럽게 끌려옴
		FVector LaunchVelocity = Direction * PullStrength * DeltaTime * 10.f;
		
		// 위로 당기는 힘을 없애고 싶다면 아래 주석 해제 (수평으로만 이동)
		// LaunchVelocity.Z = 0.f; 

		// XYOverride=false, ZOverride=false (기존 움직임에 더함)
		TargetChar->LaunchCharacter(LaunchVelocity, false, false);
	}
}

bool ASFVacuumGroundAOE::IsValidPullTarget(AActor* TargetActor) const
{
	if (!TargetActor || TargetActor == SourceActor.Get())
	{
		return false;
	}
	ACharacter* TargetChar = Cast<ACharacter>(TargetActor);
	if (!TargetChar)
	{
		return false;
	}
	
	// [중요] 비행 중(Flying)인 적은 당기지 않음 (땅으로 꺼지는 버그 방지)
	if (UCharacterMovementComponent* CMC = TargetChar->GetCharacterMovement())
	{
		if (CMC->IsFlying())
		{
			return false;
		}
	}

	if (!USFCombatLibrary::ShouldDamageTarget(SourceActor.Get(), TargetActor))
	{
		return false;
	}

	// 무적/스킬사용중 체크
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (TargetASC)
	{
		// "State.Invulnerable" (무적) 혹은 "State.UsingAbility" (스킬 사용 중) 태그가 있으면 면역
		if (TargetASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_Invulnerable) ||
			TargetASC->HasMatchingGameplayTag(SFGameplayTags::Character_State_UsingAbility))
		{
			return false;
		}
	}
	return true;
}