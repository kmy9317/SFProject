#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SFLockOnInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class USFLockOnInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 락온 시스템(LockOnComponent)이 대상을 식별하고 정보를 얻기 위해 사용하는 인터페이스
 */
class SF_API ISFLockOnInterface
{
	GENERATED_BODY()

public:
	/**
	 * 현재 이 액터가 락온 가능한 상태인지 반환합니다.
	 * @return true: 락온 가능 (생존, 타겟팅 가능 상태)
	 * @return false: 락온 불가 (사망, 은신, 무적 등)
	 */
	virtual bool CanBeLockedOn() const = 0;

	/**
	 * 락온 가능한 소켓 이름들의 목록을 반환합니다.
	 * (보스의 경우 Head, Spine, Leg 등을 반환 / 일반 몬스터는 Spine 하나만 반환)
	 */
	virtual TArray<FName> GetLockOnSockets() const = 0;

	/**
	 * 락온 되었을 때 호출 (선택 사항: UI 강조 표시 등에 활용)
	 */
	virtual void OnSelectedAsTarget(bool bSelected) {}
};