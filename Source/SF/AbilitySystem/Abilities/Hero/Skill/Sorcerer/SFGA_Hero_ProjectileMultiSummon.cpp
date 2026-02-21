#include "SFGA_Hero_ProjectileMultiSummon.h"
#include "Actors/SFAttackProjectile.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Character/SFCharacterBase.h"
#include "Algo/Sort.h"                 // 정렬 알고리즘
#include "Kismet/KismetSystemLibrary.h" // LineTrace
#include "GameFramework/PlayerController.h" // PlayerController 헤더 필요
#include "System/SFPoolSubsystem.h"

USFGA_Hero_ProjectileMultiSummon::USFGA_Hero_ProjectileMultiSummon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 기본값 설정
	NumProjectiles = 5;
	SpawnRadius = 150.f;
	ArcAngle = 360.0f;
	StartAngleOffset = 180.0f; // 뒤쪽부터 배치 시작
	bSpawnSequential = true;
	SpawnInterval = 0.1f;
	TraceDistance = 5000.0f;   // 50미터
}

void USFGA_Hero_ProjectileMultiSummon::OnProjectileSpawnEventReceived(FGameplayEventData Payload)
{
	// 1. 코스트/쿨타임 등 커밋
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		K2_CancelAbility();
		return;
	}

	// 2. 중심점(무기 위치) 계산
	FTransform CenterTM;
	if (!GetProjectileSpawnTransform(CenterTM))
	{
		if (ASFCharacterBase* Character = GetSFCharacterFromActorInfo())
		{
			CenterTM = Character->GetActorTransform();
		}
	}

	// 3. [조준] 플레이어가 바라보는 목표 지점을 계산하여 캐싱
	CachedTargetLocation = GetAimSystemTargetLocation();

	// 4. [배치] 모든 스폰 위치를 로컬 좌표(Offset)로 미리 계산
	TArray<FVector> LocalOffsets;
	float AngleStep = 0.f;
	if (NumProjectiles > 1)
	{
		bool bIsClosedLoop = FMath::IsNearlyEqual(ArcAngle, 360.0f);
		AngleStep = ArcAngle / (bIsClosedLoop ? float(NumProjectiles) : float(NumProjectiles - 1));
	}
	
	float BaseAngle = -ArcAngle * 0.5f;

	for (int32 i = 0; i < NumProjectiles; i++)
	{
		float CurrentAngle = BaseAngle + (AngleStep * i) + StartAngleOffset;
		
		// 로컬 오프셋 계산 (Forward * Radius 를 Z축 기준으로 회전)
		FVector Offset = FVector::ForwardVector * SpawnRadius;
		Offset = Offset.RotateAngleAxis(CurrentAngle, FVector::UpVector);
		
		LocalOffsets.Add(Offset);
	}

	// 5. [정렬] 위치 정렬 (뒤->앞, 우->좌)
	Algo::Sort(LocalOffsets, [](const FVector& A, const FVector& B)
	{
		// 1순위: X축 비교 (작은 X가 먼저 = 뒤에 있는 것부터)
		if (!FMath::IsNearlyEqual(A.X, B.X, 1.0f))
		{
			return A.X < B.X; 
		}
		// 2순위: Y축 비교 (큰 Y가 먼저 = 오른쪽에 있는 것부터)
		return A.Y > B.Y;
	});

	// 6. [변환] 정렬된 오프셋을 월드 좌표로 변환하여 멤버 변수에 저장
	SortedSpawnTransforms.Reset();
	for (const FVector& Offset : LocalOffsets)
	{
		// CenterTM의 회전을 적용하여 월드 위치 계산
		FVector WorldLoc = CenterTM.GetLocation() + CenterTM.GetRotation().RotateVector(Offset);
		SortedSpawnTransforms.Add(FTransform(CenterTM.GetRotation(), WorldLoc));
	}

	// 7. 발사 시작 프로세스
	CurrentSpawnCount = 0;

	if (bSpawnSequential && NumProjectiles > 0)
	{
		if (SortedSpawnTransforms.IsValidIndex(CurrentSpawnCount))
		{
			SpawnProjectileAt(SortedSpawnTransforms[CurrentSpawnCount]);
			CurrentSpawnCount++;
		}

		if (CurrentSpawnCount < NumProjectiles)
		{
			if (UWorld* World = GetWorld())
			{
				FTimerDelegate TimerDel;
				TimerDel.BindUObject(this, &ThisClass::OnSequentialSpawnTimer);

				World->GetTimerManager().SetTimer(
					SpawnTimerHandle,
					TimerDel,
					SpawnInterval,
					true // Loop
				);
			}
		}
	}
	else
	{
		// 동시 발사
		for (const FTransform& SpawnTM : SortedSpawnTransforms)
		{
			SpawnProjectileAt(SpawnTM);
		}
	}
}

void USFGA_Hero_ProjectileMultiSummon::OnSequentialSpawnTimer()
{
	if (CurrentSpawnCount >= SortedSpawnTransforms.Num())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(SpawnTimerHandle);
		}
		return;
	}

	SpawnProjectileAt(SortedSpawnTransforms[CurrentSpawnCount]);
	CurrentSpawnCount++;
}

void USFGA_Hero_ProjectileMultiSummon::SpawnProjectileAt(const FTransform& SpawnTM)
{
	if (!ProjectileClass || !HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	UWorld* World = GetWorld();
	ASFCharacterBase* Character = GetSFCharacterFromActorInfo();
	USFAbilitySystemComponent* SourceASC = GetSFAbilitySystemComponentFromActorInfo();
	if (!World || !Character || !SourceASC)
	{
		return;
	}

	FVector FinalLaunchDir = (CachedTargetLocation - SpawnTM.GetLocation()).GetSafeNormal();

	FTransform FinalTM(FinalLaunchDir.Rotation(), SpawnTM.GetLocation());
	
	USFPoolSubsystem* Pool = USFPoolSubsystem::Get(this);
	if (!Pool)
	{
		return;
	}
	ASFAttackProjectile* Projectile = Pool->AcquireActor<ASFAttackProjectile>(ProjectileClass, FinalTM);
	if (Projectile)
	{
		Projectile->SetOwner(Character);
		Projectile->SetInstigator(Cast<APawn>(Character));
		const float Damage = GetScaledBaseDamage();
		Projectile->InitProjectile(SourceASC, Damage, Character);
		Projectile->Launch(FinalLaunchDir);
	}
}

FVector USFGA_Hero_ProjectileMultiSummon::GetAimSystemTargetLocation() const
{
	APlayerController* PC = nullptr;

	if (CurrentActorInfo && CurrentActorInfo->PlayerController.IsValid())
	{
		PC = CurrentActorInfo->PlayerController.Get();
	}

	if (!PC)
	{
		// 플레이어 컨트롤러를 못 찾으면 (AI 등) 캐릭터 전방 TraceDistance 지점 반환
		if (ASFCharacterBase* Character = GetSFCharacterFromActorInfo())
		{
			return Character->GetActorLocation() + (Character->GetActorForwardVector() * TraceDistance);
		}
		return FVector::ZeroVector;
	}

	// --- 이하 로직 동일 ---

	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	FVector TraceStart = CamLoc;
	FVector TraceEnd = CamLoc + (CamRot.Vector() * TraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetAvatarActorFromActorInfo());

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	return bHit ? HitResult.ImpactPoint : TraceEnd;
}

void USFGA_Hero_ProjectileMultiSummon::EndAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,bool bReplicateEndAbility,bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}