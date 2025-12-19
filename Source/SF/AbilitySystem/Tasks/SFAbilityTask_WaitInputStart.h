#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "SFAbilityTask_WaitInputStart.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInputStartDelegate);

/**
 * 새로운 입력 시작을 감지하는 어빌리티 태스크
 * - 이벤트 기반: GameCustom1 레플리케이트 이벤트를 통한 입력 감지
 * 네트워크 처리:
 * - 클라이언트 예측: ServerSetReplicatedEvent로 서버에 알림
 * - 서버 권한: ConsumeGenericReplicatedEvent로 이벤트 소비
 * - 원격 클라이언트: 지연된 이벤트 처리를 위한 대기 상태 지원
*/
UCLASS()
class SF_API USFAbilityTask_WaitInputStart : public UAbilityTask
{
	GENERATED_BODY()

public:
	USFAbilityTask_WaitInputStart(const FObjectInitializer& ObjectInitializer);
	
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta=(HidePin="OwningAbility", DefaultToSelf="OwningAbility", BlueprintInternalUseOnly="true"))
	static USFAbilityTask_WaitInputStart* WaitInputStart(UGameplayAbility* OwningAbility);
	
public:
	virtual void Activate() override;
	
public:
	UFUNCTION()
	void OnStartCallback();

public:
	UPROPERTY(BlueprintAssignable)
	FInputStartDelegate OnStart;
	
protected:
	/**
	 * GameCustom1 이벤트에 등록된 delegate 핸들
	 * OnStartCallback 호출 후 delegate 제거를 위해 사용
	 */
	FDelegateHandle DelegateHandle;
};
