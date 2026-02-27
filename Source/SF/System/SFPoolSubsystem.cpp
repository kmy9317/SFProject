#include "SFPoolSubsystem.h"

#include "SFLogChannels.h"
#include "Interface/SFPoolable.h"

USFPoolSubsystem* USFPoolSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	return World->GetSubsystem<USFPoolSubsystem>();
}

bool USFPoolSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// PIE + Game 월드에서만 동작, Editor 프리뷰 등 제외
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void USFPoolSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PrewarmTimerHandle);
	}

	PendingPrewarms.Empty();
	
	Super::Deinitialize();
}

AActor* USFPoolSubsystem::AcquireActor(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform)
{
	if (!ActorClass)
	{
		return nullptr;
	}

	FSFPoolArray& Pool = ActorPools.FindOrAdd(ActorClass);
	AActor* Actor = nullptr;

	// 풀에서 유휴 액터 검색 (Pending Kill 체크)
	while (Pool.InactiveActors.Num() > 0)
	{
		AActor* Candidate = Pool.InactiveActors.Pop();
		if (IsValid(Candidate))
		{
			Actor = Candidate;
			break;
		}
		// 유효하지 않은 액터는 카운트 차감
		Pool.TotalSpawnedCount = FMath::Max(0, Pool.TotalSpawnedCount - 1);
	}

	// 풀이 비었으면 새로 스폰
	if (!Actor)
	{
		Actor = SpawnPooledActor(ActorClass);
	}

	if (!Actor)
	{
		UE_LOG(LogSF, Warning, TEXT("[SFPool] Failed to spawn new %s"), *ActorClass->GetName());
		return nullptr;
	}


	// ISFPoolable 미구현 경고
	if (!Cast<ISFPoolable>(Actor))
	{
		UE_LOG(LogSF, Warning, TEXT("[SFPool] %s does not implement ISFPoolable — pool state tracking disabled"), *ActorClass->GetName());
	}

	ActivateActor(Actor, SpawnTransform);
	return Actor;
}

void USFPoolSubsystem::ReturnToPool(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	// 이중 반납 방어
	ISFPoolable* Poolable = Cast<ISFPoolable>(Actor);
	if (Poolable && Poolable->IsInactiveInPool())
	{
		UE_LOG(LogSF, Warning, TEXT("[SFPool] %s is already returned to pool"), *Actor->GetName());
		return;
	}
	DeactivateActor(Actor);
	UE_LOG(LogSF, Warning, TEXT("[SFPool] %s is Deactivated from pool"), *Actor->GetName());

	FSFPoolArray& Pool = ActorPools.FindOrAdd(Actor->GetClass());
	Pool.InactiveActors.Add(Actor);
}

void USFPoolSubsystem::PrewarmPool(TSubclassOf<AActor> ActorClass, int32 Count)
{
	if (!ActorClass || Count <= 0)
	{
		return;
	}
	
	PendingPrewarms.Add({ ActorClass, Count  });

	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(PrewarmTimerHandle))
		{
			World->GetTimerManager().SetTimer(PrewarmTimerHandle, this,&USFPoolSubsystem::ProcessPrewarmBatch,0.1f, true);
		}
	}
}

void USFPoolSubsystem::ProcessPrewarmBatch()
{
	int32 SpawnedThisFrame = 0;
	while (PendingPrewarms.Num() > 0 && SpawnedThisFrame < PrewarmBatchSize)
	{
		FSFPendingPrewarm& Current = PendingPrewarms[0];
		FSFPoolArray& Pool = ActorPools.FindOrAdd(Current.ActorClass);
		if (AActor* Actor = SpawnPooledActor(Current.ActorClass))
		{
			Pool.InactiveActors.Add(Actor);
			SpawnedThisFrame++;
		}

		Current.Remaining--;
		if (Current.Remaining <= 0)
		{
			UE_LOG(LogSF, Warning, TEXT("[SFPool] Prewarm complete: %s (pool: %d)"), *Current.ActorClass->GetName(), Pool.InactiveActors.Num());
			PendingPrewarms.RemoveAt(0);
		}
	}

	if (PendingPrewarms.Num() == 0)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(PrewarmTimerHandle);
		}
		UE_LOG(LogSF, Warning, TEXT("[SFPool] All prewarm complete"));
	}
}

AActor* USFPoolSubsystem::SpawnPooledActor(TSubclassOf<AActor> ActorClass)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	
	FSFPoolArray& Pool = ActorPools.FindOrAdd(ActorClass);
	AActor* NewActor = World->SpawnActorDeferred<AActor>(ActorClass, FTransform::Identity, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!NewActor)
	{
		return nullptr;
	}
	
	// 복제 완전 차단 상태로 시작
	NewActor->SetReplicates(false);
	NewActor->bAlwaysRelevant = true;
	NewActor->FinishSpawning(FTransform::Identity);
	NewActor->SetActorEnableCollision(false);
	NewActor->SetActorHiddenInGame(true);
	NewActor->SetActorTickEnabled(false);
	NewActor->SetActorLocation(FVector(0.f, 0.f, -10000.f));

	// 풀 상태 초기화
	if (ISFPoolable* Poolable = Cast<ISFPoolable>(NewActor))
	{
		Poolable->bInactiveInPool = true;
	}

	// // Flush 없이 바로 Dormant → 초기 복제(x)
	NewActor->SetNetDormancy(DORM_DormantAll);

	Pool.TotalSpawnedCount++;
	return NewActor;
}

void USFPoolSubsystem::ActivateActor(AActor* Actor, const FTransform& SpawnTransform)
{
	if (!Actor->GetIsReplicated())
	{
		Actor->SetReplicates(true);
	}
	
	Actor->SetNetDormancy(DORM_Awake);
	Actor->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::ResetPhysics);
	Actor->SetActorHiddenInGame(false);
	Actor->SetActorTickEnabled(true);

	// 타입별 활성화 로직 위임
	if (ISFPoolable* Poolable = Cast<ISFPoolable>(Actor))
	{
		Poolable->bInactiveInPool = false;
		Poolable->OnAcquiredFromPool();
	}
	
	Actor->ForceNetUpdate();
}

void USFPoolSubsystem::DeactivateActor(AActor* Actor)
{
	// 타입별 정리
	if (ISFPoolable* Poolable = Cast<ISFPoolable>(Actor))
	{
		Poolable->bInactiveInPool = true;
		Poolable->OnReturnedToPool();
	}
	
	// Tick, 가시성, 충돌 OFF
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);
	Actor->SetActorHiddenInGame(true);
	
	// 타이머 전체 정리
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(Actor);
	}

	// 월드 밖으로 이동(시각적 안전장치)
	Actor->SetActorLocation(FVector(0.f, 0.f, -10000.f));

	// Dormancy 설정 → NetDriver 순회에서 제외
	Actor->FlushNetDormancy();
	Actor->SetNetDormancy(DORM_DormantAll);
}