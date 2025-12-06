#pragma once

#include "CoreMinimal.h"
#include "SFGA_Thrust_Base.h"
#include "SFGA_Thrust_HeartBreaker.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
/**
 * 
 */
UCLASS()
class SF_API USFGA_Thrust_HeartBreaker : public USFGA_Thrust_Base
{
	GENERATED_BODY()

public:
	USFGA_Thrust_HeartBreaker(FObjectInitializer const& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void OnTrace(FGameplayEventData Payload) override;

	float GetPhaseDamage() const;
	float GetPhaseRushDistance() const;

	UFUNCTION()
	void OnKeyReleased(float TimeHeld);
	
	void OnServerTargetDataReceivedCallback(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag);

	void SetupMotionWarpingTarget(const FVector& TargetLocation);
	void ExecuteRushAttack();

	int32 CalculatePhase(float TimeHeld) const;
	FVector CalculateRushTargetLocation() const;
	FRotator CalculateRushTargetRotation() const;
	
	UFUNCTION()
	void OnRushMontageFinished();

	void StartPhaseTimer();
	void OnPhaseTimePassed();
	void ResetCharge();

	// UI 초기화 및 숨김 처리 (bShow=true: 표시, bShow=false: 숨김)
	void BroadcastUIConstruct(bool bShow);
	// 페이즈 변경 시 색상 업데이트 메시지 전송
	void BroadcastUIRefresh(int32 NewPhaseIndex);

protected:

	// 차징 몽타주 (Start → Loop)
	UPROPERTY(EditDefaultsOnly, Category="SF|Animation")
	TObjectPtr<UAnimMontage> ChargingMontage;

	// 돌진 공격 몽타주 (Root Motion)
	UPROPERTY(EditDefaultsOnly, Category="SF|Animation")
	TObjectPtr<UAnimMontage> RushAttackMontage;

	// Phase 전환 시간 배열 ([0] Phase 1 → Phase 2 전환 시간 [1] Phase 2 → Phase 3 전환 시간)
	UPROPERTY(EditDefaultsOnly, Category="SF|PhaseInfo", EditFixedSize)
	TArray<float> PhaseTimes;

	// Phase별 UI의 프로그레스 바 색 ([0] Phase 1 색상, [1]: Phase 2 색상, [2]: Phase 3 색상)
	UPROPERTY(EditDefaultsOnly, Category="SF|PhaseInfo", EditFixedSize)
	TArray<FLinearColor> PhaseColors;

	// Phase별 데미지 배율 ([0]: Phase 1, [1]: Phase 2, [2]: Phase 3)
	UPROPERTY(EditDefaultsOnly, Category="SF|PhaseInfo")
	TArray<float> PhaseDamageMultipliers;

	// Phase별 돌진 거리 배율
	UPROPERTY(EditDefaultsOnly, Category="SF|PhaseInfo")
	TArray<float> PhaseRushDistanceScales;

	UPROPERTY(EditDefaultsOnly, Category="SF|PhaseInfo")
	float BaseRushDistance = 124.f;

	UPROPERTY(EditDefaultsOnly, Category="SF|MotionWarping")
	FName WarpTargetName = TEXT("RushTarget");

	// 슈퍼아머 GE
	UPROPERTY(EditDefaultsOnly, Category="Buff")
	TSubclassOf<UGameplayEffect> SuperArmorEffectClass;
	
private:

	FActiveGameplayEffectHandle SuperArmorEffectHandle;

	// 현재 차징 페이즈(서버는 클라값 신뢰/검증 후 적용)
	int32 CurrentPhaseIndex = 0;
	int32 MaxPhaseIndex = 0;
	
	// 총 차징 시간
	float TotalChargeTime = 0.f;

	// 서버 검증용: 어빌리티 시작 시간
	float AbilityStartTime = 0.f;

	FTimerHandle PhaseTimerHandle;

	FDelegateHandle ServerTargetDataDelegateHandle;
};
