#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SFGameplayAbility.h"
#include "USFGA_Dodge.generated.h"

/**
 * 4방향 회피 몽타주 구조체
 * (직업별로 다른 애니메이션을 할당하기 위함)
 */
USTRUCT(BlueprintType)
struct FDodgeMontageSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> ForwardMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> BackwardMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> LeftMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> RightMontage;
};

/**
 * 소울류 구르기 (클라이언트 예측 + 서버 동기화 + 4방향 락온 지원)
 */
UCLASS()
class SF_API USFGA_Dodge : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGA_Dodge(FObjectInitializer const& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 몽타주 종료 콜백
	UFUNCTION()
	void OnMontageFinished();

	// [계산] 구를 방향과 위치, 몽타주를 결정하는 함수
	// bIsLockedOn: 락온 상태 여부
	void CalculateDodgeParameters(bool bIsLockedOn, FVector& OutLocation, FRotator& OutRotation, UAnimMontage*& OutMontage) const;

	// [실행] 계산된 파라미터로 구르기 실행 (MotionWarping + PlayMontage)
	void ApplyDodge(const FVector& TargetLocation, const FRotator& TargetRotation, UAnimMontage* MontageToPlay);

	// [서버] 클라이언트 타겟 데이터 수신 핸들러
	void OnServerTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag);
	
	// Motion Warping 설정 헬퍼
	void SetupMotionWarping(const FVector& TargetLocation, const FRotator& TargetRotation);

	// 현재 입력 방향에 따른 몽타주 반환 (Local Space 기준)
	UAnimMontage* SelectMontageBasedOnInput(const FVector& InputDirection, const FRotator& ControlRotation) const;

protected:
	// =========================================================
	//  Configuration (직업별 설정 가능)
	// =========================================================

	// [New] 4방향 구르기 몽타주 세트 (기존 단일 DodgeMontage 대체)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Animation")
	FDodgeMontageSet DirectionalDodgeMontages;

	// 백스텝 몽타주 (입력 없이 사용 시)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Animation")
	TObjectPtr<UAnimMontage> BackstepMontage;
	
	// 구르기 이동 거리
	UPROPERTY(EditDefaultsOnly, Category = "SF|Dodge")
	float DodgeDistance = 500.f;

	// Motion Warping 타겟 이름 (몽타주 내 NotifyState와 일치해야 함)
	UPROPERTY(EditDefaultsOnly, Category = "SF|Dodge")
	FName WarpTargetName = FName("DodgeTarget");

private:
	FDelegateHandle ServerTargetDataDelegateHandle;
};