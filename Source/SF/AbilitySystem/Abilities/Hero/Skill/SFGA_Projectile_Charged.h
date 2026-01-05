// SFGA_Projectile_Charged.h

#pragma once

#include "CoreMinimal.h"
#include "SFGA_Hero_ProjectileLaunch.h"
#include "SFGA_Projectile_Charged.generated.h"

class USFCameraMode;

USTRUCT(BlueprintType)
struct FSFProjectileChargePhaseInfo
{
	GENERATED_BODY()

	// 다음 페이즈까지 필요한 시간
	UPROPERTY(EditDefaultsOnly, Category="Charging", meta=(ClampMin="0.0"))
	float ChargeTimeToNext = 0.5f;

	// UI 표시 색상
	UPROPERTY(EditDefaultsOnly, Category="UI")
	FLinearColor PhaseColor = FLinearColor::White;

	// 데미지 배율
	UPROPERTY(EditDefaultsOnly, Category="Combat", meta=(ClampMin="0.0"))
	float DamageMultiplier = 1.0f;

	// 투사체 크기 (Scale)
	UPROPERTY(EditDefaultsOnly, Category="Combat", meta=(ClampMin="0.1"))
	float ProjectileScale = 1.0f;

	// 추가 폭발 여부
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	bool bEnableExplosion = false;
    
    // 이 페이즈의 카메라 모드 (선택 사항)
	UPROPERTY(EditDefaultsOnly, Category="Camera")
	TSubclassOf<USFCameraMode> CameraMode;
};

/**
 * 차징형 원거리 발사체 스킬
 */
UCLASS()
class SF_API USFGA_Projectile_Charged : public USFGA_Hero_ProjectileLaunch
{
	GENERATED_BODY()

public:
	USFGA_Projectile_Charged(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

    // 부모의 OnProjectileSpawnEventReceived를 오버라이드하여 차징된 스펙으로 발사
	virtual void OnProjectileSpawnEventReceived(FGameplayEventData Payload) override;

protected:
	// 차징 관련 로직 (HeartBreaker 참조)
	UFUNCTION()
	void OnKeyReleased(float TimeHeld);

	void StartPhaseTimer();
	void OnPhaseTimePassed();
	void ResetCharge();
	int32 CalculatePhase(float TimeHeld) const;

	void BroadcastUIConstruct(bool bShow);
	void BroadcastUIRefresh(int32 NewPhaseIndex);

    // 서버 데이터 수신
    void OnServerTargetDataReceivedCallback(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag);
    
    // 실제 발사 애니메이션 재생 (차징 종료 후)
    void PlayLaunchMontage();

protected:
	// 페이즈 정보
	UPROPERTY(EditDefaultsOnly, Category="SF|PhaseInfo")
	TArray<FSFProjectileChargePhaseInfo> PhaseInfos;

	// 차징 몽타주 (루프)
	UPROPERTY(EditDefaultsOnly, Category="SF|Animation")
	TObjectPtr<UAnimMontage> ChargingMontage;

    // 차징 중 큐 태그
    UPROPERTY(EditDefaultsOnly, Category="SF|GameplayCue")
	FGameplayTag ChargingCueTag;

private:
	// 현재 차징 페이즈
	int32 CurrentPhaseIndex = 0;
	int32 MaxPhaseIndex = 0;

	float TotalChargeTime = 0.f;
    float AbilityStartTime = 0.f;

	FTimerHandle PhaseTimerHandle;
    FDelegateHandle ServerTargetDataDelegateHandle;
};