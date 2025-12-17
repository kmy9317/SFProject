#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "SFAbilityTask_UpdateWarpTarget.generated.h"

class ASFCharacterBase;
class UMotionWarpingComponent;

// Windup 구간이 끝날 때마다 호출
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWarpTargetCommitted, FVector, CommittedDirection, FVector, CommittedLocation);

/**
 * Windup 구간 동안 Motion Warping Target을 지속적으로 업데이트하는 AbilityTask
 * - Combat.Phase.Windup 태그가 있을 때만 방향 업데이트
 * - 태그가 사라지면 일시정지, 다시 붙으면 재개
 * - GA 종료 시 또는 명시적 Commit 호출 시 최종 확정
 */
UCLASS()
class SF_API USFAbilityTask_UpdateWarpTarget : public UAbilityTask
{
	GENERATED_BODY()

public:
	USFAbilityTask_UpdateWarpTarget(const FObjectInitializer& ObjectInitializer);

	/**
	 * 태스크 생성 팩토리 함수
	 * @param OwningAbility - 이 태스크를 소유하는 어빌리티
	 * @param WarpTargetName - Motion Warping에서 사용할 타겟 이름
	 * @param WeaponRange - 무기별 거리 (Warp 목표 위치 계산에 사용)
	 * @param RotationInterpSpeed - 회전 보간 속도 (높을수록 빠른 반응, 낮을수록 묵직함)
	 * @param MaxTurnAngle - 선딜 중 최대 회전 가능 각도 (0이면 제한 없음)
	 */
	UFUNCTION(BlueprintCallable, Category = "SF|AbilityTask", meta = (DisplayName = "Update Warp Target During Windup", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static USFAbilityTask_UpdateWarpTarget* CreateTask(UGameplayAbility* OwningAbility, FName WarpTargetName, float WeaponRange = 200.f, float RotationInterpSpeed = 10.f, float MaxTurnAngle = 90.f);

	// 락온 타겟 설정 (락온 시스템과 연동)
	UFUNCTION(BlueprintCallable, Category = "SF|AbilityTask")
	void SetLockedTarget(AActor* InTarget);

	UFUNCTION(BlueprintCallable, Category = "SF|AbilityTask")
	void ClearLockedTarget();

	UFUNCTION(BlueprintPure, Category = "SF|AbilityTask")
	FVector GetCurrentWarpDirection() const { return CurrentWarpDirection; }

	UFUNCTION(BlueprintPure, Category = "SF|AbilityTask")
	FVector GetCurrentWarpLocation() const { return CurrentWarpLocation; }
	
protected:
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

private:

	void ResetInitialDirection();
	void UpdateWarpTargetFromInput(float DeltaTime);
	void UpdateWarpTargetFromLockedTarget(float DeltaTime);

	// 최대 회전 각도 제한 적용
	FVector ClampDirectionToMaxAngle(const FVector& DesiredDirection) const;
	
	FVector InterpolateDirection(const FVector& CurrentDir, const FVector& TargetDir, float DeltaTime) const;
	void ApplyWarpTarget(const FVector& Location, const FRotator& Rotation);

	FVector GetPlayerInputDirection() const;

	// Windup 태그 체크 (태그 없으면 자동 종료)
	bool IsInWindupPhase() const;
	
public:
	
	UPROPERTY(BlueprintAssignable)
	FOnWarpTargetCommitted OnWarpTargetCommitted;

private:
	
	UPROPERTY()
	TWeakObjectPtr<UMotionWarpingComponent> MotionWarpingComp;

	UPROPERTY()
	TWeakObjectPtr<ASFCharacterBase> OwnerCharacter;

	UPROPERTY()
	TWeakObjectPtr<AActor> LockedTarget;

	FName TargetName;
	float Range;
	float InterpSpeed;
	float MaxAngle;
	
	FVector CurrentWarpDirection;
	FVector CurrentWarpLocation;
	FVector InitialForwardDirection;

	// 일시정지 상태 (Windup 태그 없을 때)
	bool bIsPaused;
};
