#pragma once

#include "CoreMinimal.h"
#include "SFGA_Interact_Info.h"
#include "Animation/Hero/SFHeroAnimationData.h"
#include "SFGA_Interact_Active.generated.h"

class UInputAction;

/**
 * 홀딩 상호작용을 처리하는 핵심 어빌리티 클래스
 * 플레이어가 상호작용 키를 누르고 있는 동안 지속시간을 관리하고 진행률을 추적
 * 즉시 실행 vs 홀딩 분기 결정
 * 취소 조건 모니터링 (입력 해제, 위치 이탈)
 * 지속시간 완료 시 네트워크 동기화
 */
UCLASS()
class SF_API USFGA_Interact_Active : public USFGA_Interact_Info
{
	GENERATED_BODY()

public:
	USFGA_Interact_Active(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	
	 // 플레이어가 상호작용 대상에서 너무 멀어지거나 각도가 벗어났을 때 홀딩 취소
	UFUNCTION()
	void OnInvalidInteraction();

	void StartHoldingInteraction();

	void SendProgressMessage();
	void SendInteractingMessage();

	/** ISFInteractable에 홀딩 시작 알림 */
	void NotifyInteractableActiveStarted();

	/** ISFInteractable에 홀딩 종료 알림 */
	void NotifyInteractableActiveEnded();

	// 상호작용 입력이 해제되었을 때 호출되는 콜백
	UFUNCTION()
	void OnInputReleased(float TimeHeld);
	
	// 홀딩 지속시간이 완료되었을 때 호출되는 콜백(네트워크 동기화 시작)
	UFUNCTION()
	void OnDurationEnded();

	UFUNCTION()
	void OnNetSync();

	// 실제 상호작용을 트리거하는 함수(홀딩 완료 후 상호작용 대상의 어빌리티를 활성화)
	UFUNCTION()
	bool TriggerInteraction();

	// GaugeBased 완료 이벤트 대기 
	void WaitForGaugeBasedComplete();

	// GaugeBased 완료 이벤트 수신 시 호출 
	UFUNCTION()
	void OnGaugeBasedCompleted(FGameplayEventData Payload);

	// 캐릭터의 HeroAnimationData에서 상호작용 시작 몽타주 조회
	FSFMontagePlayData GetInteractionStartMontage() const;

protected:

	// 홀딩 시작 시 기존 움직임 입력을 플러시하기 위해 사용
	UPROPERTY(EditDefaultsOnly, Category="SF|Interaction")
	TObjectPtr<UInputAction> MoveInputAction;
	
	// 플레이어가 상호작용 대상을 바라보는 각도가 이 값을 벗어나면 홀딩 취소 (도 단위)
	UPROPERTY(EditDefaultsOnly, Category="SF|Interaction")
	float AcceptanceAngle = 65.f;

	// 상호작용 허용 거리, 이 값을 초과하면 홀딩 취소 (cm 단위)
	UPROPERTY(EditDefaultsOnly, Category="SF|Interaction")
	float AcceptanceDistance = 10.f;

private:

	FTimerHandle HoldingTimerHandle;
};
