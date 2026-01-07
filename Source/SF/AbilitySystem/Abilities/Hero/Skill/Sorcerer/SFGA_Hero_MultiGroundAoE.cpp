#include "SFGA_Hero_MultiGroundAoE.h"
#include "AbilitySystem/Abilities/Hero/Skill/SkillActor/SFMultiGroundActor.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h" // 랜덤 위치 계산용

USFGA_Hero_MultiGroundAoE::USFGA_Hero_MultiGroundAoE(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 필요 시 기본값 재정의
	AOERadius = 500.f; // 번개가 떨어질 전체 범위
}

void USFGA_Hero_MultiGroundAoE::OnSpawnEventReceived(FGameplayEventData Payload)
{
	// 부모의 로직(단일 소환)은 실행하지 않음 (Super 호출 X)

	if (LightningCount <= 0 || LightningSpawnInterval <= 0.f)
	{
		// 설정 오류 시 즉시 종료
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	CurrentLightningCount = 0;

	// 첫 발 즉시 발사
	SpawnSingleLightning();

	// 나머지는 타이머로 발사
	if (CurrentLightningCount < LightningCount)
	{
		GetWorld()->GetTimerManager().SetTimer(
			LightningTimerHandle,
			this,
			&USFGA_Hero_MultiGroundAoE::SpawnSingleLightning,
			LightningSpawnInterval,
			true // 반복
		);
	}
	else
	{
		// 1발짜리 스킬인 경우 바로 종료 처리 필요할 수 있음
		// 다만 몽타주가 계속 재생 중이면 EndAbility는 몽타주 종료 시점에 호출됨
	}
}

void USFGA_Hero_MultiGroundAoE::SpawnSingleLightning()
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		// 클라이언트는 연출만 필요하다면 GC 등을 사용해야 하지만,
		// 여기서는 서버에서 액터를 스폰하여 리플리케이션되는 구조로 가정합니다.
		return;
	}

	// 횟수 체크
	if (CurrentLightningCount >= LightningCount)
	{
		GetWorld()->GetTimerManager().ClearTimer(LightningTimerHandle);
		return;
	}

	CurrentLightningCount++;

	if (LightningActorClass && GetWorld())
	{
		// 1. 랜덤 위치 계산
		// TargetLocation(부모 클래스에서 Reticle로 정한 중심점) 기준 랜덤 포인트
		FVector2D RandomPoint = FMath::RandPointInCircle(AOERadius);
		FVector SpawnLocation = TargetLocation + FVector(RandomPoint.X, RandomPoint.Y, 0.0f);

		// 지면 높이 보정 (LineTrace 등을 통해 정확한 지면을 찾을 수도 있음)
		// 여기서는 TargetLocation Z값을 그대로 사용

		// 2. 랜덤 회전 (Y축 회전 등 필요한 경우)
		FRotator SpawnRotation = FRotator::ZeroRotator;

		FTransform SpawnTransform(SpawnRotation, SpawnLocation);

		FActorSpawnParameters Params;
		Params.Owner = GetAvatarActorFromActorInfo();
		Params.Instigator = Cast<APawn>(Params.Owner);
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// 3. 액터 소환
		ASFMultiGroundActor* Lightning = GetWorld()->SpawnActor<ASFMultiGroundActor>(LightningActorClass, SpawnTransform, Params);
		if (Lightning)
		{
			// 4. 랜덤 크기 설정
			float RandomScale = FMath::RandRange(MinScaleMultiplier, MaxScaleMultiplier);
			
			// 크기에 비례하여 데미지도 조절할지? (요구사항엔 없으므로 크기만 조절)
			// 여기서는 시각적 크기와 충돌체 크기 모두 반영
			
			float FinalDamage = BaseDamage.GetValueAtLevel(GetAbilityLevel());
			float FinalRadius = LightningBoltRadius * RandomScale;

			Lightning->SetActorScale3D(FVector(RandomScale)); // 전체 스케일 적용

			// 초기화 호출
			Lightning->InitLightning(
				GetAbilitySystemComponentFromActorInfo(),
				GetAvatarActorFromActorInfo(),
				FinalDamage,
				FinalRadius,
				LightningBoltHeight
			);
		}
	}
}

void USFGA_Hero_MultiGroundAoE::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 어빌리티 종료 시(캔슬 등) 타이머 정리
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(LightningTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}