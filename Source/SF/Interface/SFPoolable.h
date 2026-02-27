#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SFPoolable.generated.h"

// This class does not need to be modified.
UINTERFACE()
class USFPoolable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SF_API ISFPoolable
{
	GENERATED_BODY()

public:
	// 풀에서 꺼내어 월드에 배치될 때 호출 — 타입별 초기화 로직
	virtual void OnAcquiredFromPool() {}

	// 풀로 반환되기 직전에 호출 — 타입별 정리 로직
	virtual void OnReturnedToPool() {}

	bool IsInactiveInPool() const { return bInactiveInPool; }

protected:
	friend class USFPoolSubsystem;
	
	bool bInactiveInPool = false;
};
