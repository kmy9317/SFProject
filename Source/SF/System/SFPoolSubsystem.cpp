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

	FSFPoolArray& Pool = ActorPools.FindOrAdd(ActorClass);

	for (int32 i = 0; i < Count; ++i)
	{
		AActor* Actor = SpawnPooledActor(ActorClass);
		if (Actor)
		{
			Actor->SetNetDormancy(DORM_DormantAll);
			Pool.InactiveActors.Add(Actor);
		}
	}
}

void USFPoolSubsystem::SetPoolLimit(TSubclassOf<AActor> ActorClass, int32 MaxCount)
{
	if (ActorClass)
	{
		PoolLimits.Add(ActorClass, MaxCount);
	}
}

AActor* USFPoolSubsystem::SpawnPooledActor(TSubclassOf<AActor> ActorClass)
{
	FSFPoolArray& Pool = ActorPools.FindOrAdd(ActorClass);

	// 하드 리밋 체크
	const int32 MaxCount = PoolLimits.Contains(ActorClass) ? PoolLimits[ActorClass] : DefaultMaxPoolSize;

	if (Pool.TotalSpawnedCount >= MaxCount)
	{
		UE_LOG(LogSF, Warning, TEXT("[SFPool] Hard limit reached for %s (%d/%d)"), *ActorClass->GetName(), Pool.TotalSpawnedCount, MaxCount);
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* NewActor = World->SpawnActor<AActor>(ActorClass, FTransform::Identity, Params);
	if (!NewActor)
	{
		return nullptr;
	}


	NewActor->bAlwaysRelevant = true;
	NewActor->SetActorEnableCollision(false);
	NewActor->SetActorHiddenInGame(true);
	NewActor->SetActorTickEnabled(false);

	Pool.TotalSpawnedCount++;

	UE_LOG(LogSF, Warning, TEXT("[SFPool] Spawned new %s (Total: %d/%d)"), *ActorClass->GetName(), Pool.TotalSpawnedCount, MaxCount);

	return NewActor;
}

void USFPoolSubsystem::ActivateActor(AActor* Actor, const FTransform& SpawnTransform)
{
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