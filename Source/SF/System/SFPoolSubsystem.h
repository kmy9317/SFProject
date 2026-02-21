#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SFPoolSubsystem.generated.h"

USTRUCT()
struct FSFPoolArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> InactiveActors;

	int32 TotalSpawnedCount = 0;
};

/**
 * 
 */
UCLASS()
class SF_API USFPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static USFPoolSubsystem* Get(const UObject* WorldContextObject);

	// 풀에서 액터를 꺼내거나 없으면 새로 스폰하여 반환
	AActor* AcquireActor(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform);
	
	template<typename T>
	T* AcquireActor(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform)
	{
		return Cast<T>(AcquireActor(ActorClass, SpawnTransform));
	}

	// 액터를 풀로 반환 (Destroy 대신 호출)
	void ReturnToPool(AActor* Actor);

	// 지정 클래스의 액터를 Count개만큼 미리 스폰
	void PrewarmPool(TSubclassOf<AActor> ActorClass, int32 Count);

	// 클래스별 하드 리밋 설정
	void SetPoolLimit(TSubclassOf<AActor> ActorClass, int32 MaxCount);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	AActor* SpawnPooledActor(TSubclassOf<AActor> ActorClass);
	void ActivateActor(AActor* Actor, const FTransform& SpawnTransform);
	void DeactivateActor(AActor* Actor);

	UPROPERTY()
	TMap<TSubclassOf<AActor>, FSFPoolArray> ActorPools;

	// 클래스별 최대 스폰 수 (하드 리밋), 미등록 시 DefaultMaxPoolSize 사용
	TMap<TSubclassOf<AActor>, int32> PoolLimits;

	int32 DefaultMaxPoolSize = 50;
};
